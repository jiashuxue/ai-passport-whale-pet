// main/main.c —— 「小鲸 · DeepSeek 鲸鱼娘」宠物应用入口。
//
// 按键语义(全局统一):
//   上  短按   喂食
//   下  短按   玩耍
//   确定 短按  摸摸
//   确定 长按  打开/关闭「关于小鲸」状态页
//
// 按键回调只入队;UI 只在 LVGL 任务里、持 bsp_lvgl_lock 时访问。
// 存档通过 NVS 保存(命名空间 "whale"),由 input 任务节流写入。
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"      // 错误日志里要打印 BSP_LCD_* 引脚号
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "key_guard.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pet_state.h"
#include "pet_ui.h"

static const char *TAG = "whale";

#define INPUT_QUEUE_DEPTH 8
#define SAVE_MIN_INTERVAL_MS 5000
#define NVS_NAMESPACE "whale"
#define NVS_KEY_PET "pet"

// 深睡按键唤醒会让芯片在"唤醒键还按着"的状态下重启,先挡住它的动作(见 on_key)。
#define KEY_GUARD_MAX_US (10LL * 1000 * 1000)

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t event;
} input_event_t;

static pet_state_t s_pet;
static key_guard_t s_key_guard;
static QueueHandle_t s_input_queue;
static TaskHandle_t s_input_task;
static volatile bool s_input_ready;

// button callbacks run on the shared esp_timer task; enqueue only and return immediately.
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    // Consume the guard before the readiness check: a release can arrive after the
    // post-init voltage sample but before input dispatch is ready, and dropping it
    // here would leave the guard armed to swallow the next real press.
    const bool was_armed = s_key_guard.armed;
    if (key_guard_consume(&s_key_guard, ev == BSP_BTN_RELEASE, esp_timer_get_time())) {
        if (was_armed && !s_key_guard.armed) {
            ESP_LOGI(TAG, "唤醒键已松手,按键恢复响应");
        }
        return;
    }
    if (!s_input_ready || !s_input_queue) {
        return;
    }
    const input_event_t input = { .btn = btn, .event = ev };
    (void)xQueueSend(s_input_queue, &input, 0);
}

// ---------------- NVS 存档 ----------------
static esp_err_t save_state(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    uint8_t blob[PET_BLOB_SIZE];
    size_t len = pet_state_save(&s_pet, blob, sizeof(blob));
    err = nvs_set_blob(handle, NVS_KEY_PET, blob, len);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static void load_state(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }
    size_t len = PET_BLOB_SIZE;
    uint8_t blob[PET_BLOB_SIZE];
    if (nvs_get_blob(handle, NVS_KEY_PET, blob, &len) == ESP_OK &&
        pet_state_load(&s_pet, blob, len)) {
        ESP_LOGI(TAG, "读档:mood=%d fullness=%d affection=%d 互动=%lu",
                 s_pet.mood, s_pet.fullness, s_pet.affection,
                 (unsigned long)s_pet.total_interactions);
    } else {
        ESP_LOGI(TAG, "无有效存档,使用初始状态");
    }
    nvs_close(handle);
}

// ---------------- 输入分发 ----------------
static void input_task(void *arg)
{
    (void)arg;
    input_event_t input;
    uint32_t last_save_ms = 0;
    for (;;) {
        if (xQueueReceive(s_input_queue, &input, pdMS_TO_TICKS(1000)) == pdTRUE) {
            pet_ui_key(input.btn, input.event);
        }
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (pet_ui_minute_tick() && now_ms - last_save_ms >= SAVE_MIN_INTERVAL_MS) {
            if (save_state() == ESP_OK) {
                last_save_ms = now_ms;
            }
        }
    }
}

static esp_err_t input_dispatch_init(void)
{
    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (!s_input_queue) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(input_task, "pet_input", 4096, NULL, 5, &s_input_task) != pdPASS) {
        vQueueDelete(s_input_queue);
        s_input_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

// ---------------- 入口 ----------------
void app_main(void)
{
    ESP_LOGI(TAG, "小鲸 · DeepSeek 鲸鱼娘 启动");
    const uint32_t wake_causes = esp_sleep_get_wakeup_causes();
    if (wake_causes & ~(1u << ESP_SLEEP_WAKEUP_UNDEFINED)) {
        ESP_LOGI(TAG, "休眠唤醒原因位图: 0x%02x", (unsigned)wake_causes);
    }
    // 按键唤醒时,那颗键在整个启动期间还按着。先挡住它的动作(见 on_key)。
    if (wake_causes & (1u << ESP_SLEEP_WAKEUP_GPIO)) {
        key_guard_arm(&s_key_guard, esp_timer_get_time(), KEY_GUARD_MAX_US);
        ESP_LOGI(TAG, "按键唤醒:先屏蔽按键动作,直到那颗键松手");
    }

    bsp_i2c_init();

    // 屏幕是 UI 载体,失败就无法继续。
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败,应用无法继续。"
                      "检查 SPI 接线(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    // NVS 与存档(分区表损坏时按 IDF 惯例修复,这会清空已有存档)。
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS 需要修复,将清空存储");
        nvs_flash_erase();
        nvs_err = nvs_flash_init();
    }
    pet_state_init(&s_pet);
    if (nvs_err == ESP_OK) {
        load_state();
    }

    // 按键与输入分发。
    esp_err_t input_err = input_dispatch_init();
    if (input_err == ESP_OK) {
        esp_err_t button_err = bsp_button_init(on_key, NULL);
        if (button_err != ESP_OK) {
            ESP_LOGE(TAG, "按键初始化失败: %s", esp_err_to_name(button_err));
        }
        if (s_key_guard.armed) {
            // 唤醒键若在按键驱动启动前就松手,按真实电压同步一次立即撤防。
            const bool released = bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN;
            key_guard_sync_released(&s_key_guard, released);
        }
    } else {
        ESP_LOGE(TAG, "输入分发初始化失败: %s", esp_err_to_name(input_err));
    }

    // 电量计可选,失败不影响使用(UI 会显示 --)。
    if (bsp_battery_init() != ESP_OK) {
        ESP_LOGI(TAG, "电量计不可用,UI 显示 --");
    }

    // UI 启动。
    const esp_app_desc_t *desc = esp_app_get_description();
    if (bsp_lvgl_lock(1000)) {
        pet_ui_init(&s_pet, desc ? desc->version : "");
        bsp_lvgl_unlock();
        s_input_ready = true;
    }

    ESP_LOGI(TAG, "就绪:mood=%d fullness=%d affection=%d",
             s_pet.mood, s_pet.fullness, s_pet.affection);
}
