// main/pet_ui.c —— 「小鲸」界面实现。
// 设计语言:DeepSeek 深蓝 + 白色卡片 + 彩色状态条,全中文界面(app_font_16/20)。
// 动画系统:循环组(待机/睡眠) + 一次性动作组,素材来自 dsh-whale-girl-pet(MIT)。
#include "pet_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "lvgl.h"
#include <stdio.h>

LV_FONT_DECLARE(app_font_16);
LV_FONT_DECLARE(app_font_20);

// ---------------- 立绘帧(EMBED_FILES 生成的二进制,存于 flash) ----------------
#define WHALE_DECLARE(name)                                                     \
    extern const uint8_t whale_##name##_data[]                                  \
        asm("_binary_whale_" #name "_rgb565_start");

#define WHALE_DSC(name)                                                         \
    {                                                                           \
        .header = {                                                             \
            .cf = LV_COLOR_FORMAT_RGB565,                                       \
            .w = 160,                                                           \
            .h = 160,                                                           \
            .stride = 160 * 2,                                                  \
        },                                                                      \
        .data_size = 160 * 160 * 2,                                             \
        .data = whale_##name##_data,                                            \
    }

WHALE_DECLARE(idle_0)
WHALE_DECLARE(idle_1)
WHALE_DECLARE(happy_0)
WHALE_DECLARE(happy_1)
WHALE_DECLARE(happy_2)
WHALE_DECLARE(shy_0)
WHALE_DECLARE(shy_1)
WHALE_DECLARE(shy_2)
WHALE_DECLARE(tsun_0)
WHALE_DECLARE(tsun_1)
WHALE_DECLARE(tsun_2)
WHALE_DECLARE(eat_0)
WHALE_DECLARE(eat_1)
WHALE_DECLARE(eat_2)
WHALE_DECLARE(play_0)
WHALE_DECLARE(play_1)
WHALE_DECLARE(play_2)
WHALE_DECLARE(sleep_0)
WHALE_DECLARE(sleep_1)
WHALE_DECLARE(sleeping_0)
WHALE_DECLARE(sleeping_1)
WHALE_DECLARE(wake_0)
WHALE_DECLARE(wake_1)
WHALE_DECLARE(wake_2)
WHALE_DECLARE(stretch_0)
WHALE_DECLARE(stretch_1)
WHALE_DECLARE(stretch_2)
WHALE_DECLARE(look_0)
WHALE_DECLARE(look_1)
WHALE_DECLARE(look_2)
WHALE_DECLARE(hum_0)
WHALE_DECLARE(hum_1)
WHALE_DECLARE(hum_2)

// ---------------- 动画组 ----------------
typedef struct {
    const lv_image_dsc_t *frames;
    uint8_t count;
} anim_group_t;

static const lv_image_dsc_t s_f_idle[2] = {WHALE_DSC(idle_0), WHALE_DSC(idle_1)};
static const lv_image_dsc_t s_f_happy[3] = {WHALE_DSC(happy_0), WHALE_DSC(happy_1), WHALE_DSC(happy_2)};
static const lv_image_dsc_t s_f_shy[3] = {WHALE_DSC(shy_0), WHALE_DSC(shy_1), WHALE_DSC(shy_2)};
static const lv_image_dsc_t s_f_tsun[3] = {WHALE_DSC(tsun_0), WHALE_DSC(tsun_1), WHALE_DSC(tsun_2)};
static const lv_image_dsc_t s_f_eat[3] = {WHALE_DSC(eat_0), WHALE_DSC(eat_1), WHALE_DSC(eat_2)};
static const lv_image_dsc_t s_f_play[3] = {WHALE_DSC(play_0), WHALE_DSC(play_1), WHALE_DSC(play_2)};
static const lv_image_dsc_t s_f_sleep[2] = {WHALE_DSC(sleep_0), WHALE_DSC(sleep_1)};
static const lv_image_dsc_t s_f_sleeping[2] = {WHALE_DSC(sleeping_0), WHALE_DSC(sleeping_1)};
static const lv_image_dsc_t s_f_wake[3] = {WHALE_DSC(wake_0), WHALE_DSC(wake_1), WHALE_DSC(wake_2)};
static const lv_image_dsc_t s_f_stretch[3] = {WHALE_DSC(stretch_0), WHALE_DSC(stretch_1), WHALE_DSC(stretch_2)};
static const lv_image_dsc_t s_f_look[3] = {WHALE_DSC(look_0), WHALE_DSC(look_1), WHALE_DSC(look_2)};
static const lv_image_dsc_t s_f_hum[3] = {WHALE_DSC(hum_0), WHALE_DSC(hum_1), WHALE_DSC(hum_2)};
static const lv_image_dsc_t s_f_about = WHALE_DSC(idle_0);

static const anim_group_t g_idle = {s_f_idle, 2};
static const anim_group_t g_sleeping = {s_f_sleeping, 2};
static const anim_group_t g_happy = {s_f_happy, 3};
static const anim_group_t g_shy = {s_f_shy, 3};
static const anim_group_t g_tsun = {s_f_tsun, 3};
static const anim_group_t g_eat = {s_f_eat, 3};
static const anim_group_t g_play = {s_f_play, 3};
static const anim_group_t g_sleep = {s_f_sleep, 2};
static const anim_group_t g_wake = {s_f_wake, 3};
static const anim_group_t g_stretch = {s_f_stretch, 3};
static const anim_group_t g_look = {s_f_look, 3};
static const anim_group_t g_hum = {s_f_hum, 3};

// ---------------- 配色 ----------------
#define C_INK 0x16307C
#define C_BG 0xEAF6FF
#define C_CARD 0xFFFFFF
#define C_SLOT 0xD9E7EC
#define C_MOOD 0xFF8FA3
#define C_FULL 0xFFB23E
#define C_AFF 0x4D6BFE

// ---------------- 台词池 ----------------
static const char *const LINES_HAPPY[] = {
    "嘿嘿~好舒服~",
    "再摸摸我嘛~",
};
static const char *const LINES_SHY[] = {
    "呀！别突然摸我啦~",
};
static const char *const LINES_TSUN[] = {
    "哼！才不是想被摸~",
};
static const char *const LINES_FEED[] = {
    "小鱼干真好吃！",
    "谢谢投喂~",
    "吃饱饱啦~",
};
static const char *const LINES_PLAY[] = {
    "一起来玩呀~",
    "看我跳高高！",
    "好开心呀~",
};
static const char *const LINES_HUNGRY[] = {
    "想吃小鱼干~",
};
static const char *const LINES_LOW[] = {
    "陪我说说话嘛~",
    "有点小失落…",
};
static const char *const LINES_IDLE[] = {
    "今天也要加油鸭~",
    "小鲸在这里哦~",
    "摸摸我嘛~",
};
static const char *const LINE_LEVELUP = "哇，升级啦！";
static const char *const LINE_WELCOME = "欢迎回来~";
static const char *const LINE_SLEEPIN = "呼…晚安~";
static const char *const LINE_WAKE = "呜…被吵醒了~";

static const char *pick(const char *const *lines, size_t count)
{
    return lines[esp_random() % count];
}

// ---------------- 界面状态 ----------------
static pet_state_t *s_state;
static char s_fw_version[32];

static lv_obj_t *s_main_scr;
static lv_obj_t *s_about_scr;
static lv_obj_t *s_img;
static lv_obj_t *s_bubble;
static lv_obj_t *s_battery_label;
static lv_obj_t *s_bar_mood;
static lv_obj_t *s_bar_full;
static lv_obj_t *s_bar_aff;
static lv_obj_t *s_about_level;
static lv_obj_t *s_about_count;

static lv_timer_t *s_anim_timer;
static lv_timer_t *s_bubble_timer;
static lv_timer_t *s_minute_timer;
static lv_timer_t *s_battery_timer;
static lv_timer_t *s_idle_timer;

// 动画运行时:循环组 + 一次性组
static const anim_group_t *s_loop;    // 当前循环组(idle 或 sleeping)
static uint8_t s_loop_idx;
static uint8_t s_loop_sub;            // 循环组换帧分频
static const anim_group_t *s_once;    // 一次性动作组(NULL=无)
static uint8_t s_once_ticks;
static bool s_asleep;
static volatile bool s_save_pending;

static void minute_cb(lv_timer_t *timer);
static void battery_cb(lv_timer_t *timer);
static void idle_act_cb(lv_timer_t *timer);

// ---------------- 小工具 ----------------
static lv_obj_t *block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, int x, int y, const char *text,
                       const lv_font_t *font, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

// ---------------- 气泡 ----------------
static void bubble_clear_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_bubble) {
        lv_label_set_text(s_bubble, "");
    }
}

static void bubble_show(const char *text)
{
    if (!s_bubble) {
        return;
    }
    lv_label_set_text(s_bubble, text);
    if (s_bubble_timer) {
        lv_timer_reset(s_bubble_timer);
    }
}

static void bubble_show_state_lines(void)
{
    pet_bubble_t kind = pet_state_pick_bubble(s_state,
                                              (uint32_t)(esp_timer_get_time() / 1000));
    switch (kind) {
    case PET_BUBBLE_HUNGRY:
        bubble_show(pick(LINES_HUNGRY, sizeof(LINES_HUNGRY) / sizeof(LINES_HUNGRY[0])));
        break;
    case PET_BUBBLE_LOW_MOOD:
        bubble_show(pick(LINES_LOW, sizeof(LINES_LOW) / sizeof(LINES_LOW[0])));
        break;
    case PET_BUBBLE_SLEEPY:
    case PET_BUBBLE_NONE:
    default:
        bubble_show(pick(LINES_IDLE, sizeof(LINES_IDLE) / sizeof(LINES_IDLE[0])));
        break;
    }
}

// ---------------- 动画播放器 ----------------
static void anim_set_frame(const anim_group_t *g, uint8_t idx)
{
    if (s_img && g) {
        lv_image_set_src(s_img, &g->frames[idx % g->count]);
    }
}

static void anim_once(const anim_group_t *g)
{
    s_once = g;
    s_once_ticks = 0;
    anim_set_frame(g, 0);
}

static void anim_set_loop(const anim_group_t *g)
{
    s_loop = g;
    s_loop_idx = 0;
    s_loop_sub = 0;
    anim_set_frame(g, 0);
}

static void anim_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_img) {
        return;
    }
    // tick = 250ms; 换帧分频 2 -> 500ms/帧。
    // 一次性动作(3 帧)播 2 轮约 3 秒; 2 帧组约 2 秒; 循环组呼吸周期 1 秒。
    if (s_once) {
        s_once_ticks++;
        uint8_t f = s_once_ticks / 2;
        if (f >= s_once->count * 2) {
            s_once = NULL;
            anim_set_frame(s_loop, 0);
            s_loop_idx = 0;
            s_loop_sub = 0;
        } else {
            anim_set_frame(s_once, f % s_once->count);
        }
    } else if (s_loop) {
        if (++s_loop_sub >= 2) {
            s_loop_sub = 0;
            s_loop_idx ^= 1;
            anim_set_frame(s_loop, s_loop_idx);
        }
    }
}

// ---------------- 睡眠 ----------------
static void pet_fall_asleep(void)
{
    s_asleep = true;
    anim_set_loop(&g_sleeping);
    anim_once(&g_sleep);
    bubble_show(LINE_SLEEPIN);
    s_save_pending = true;
}

static void pet_wake_up(void)
{
    s_asleep = false;
    anim_set_loop(&g_idle);
    anim_once(&g_wake);
    bubble_show(LINE_WAKE);
}

// ---------------- 待机小动作 ----------------
static void idle_act_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_asleep || s_once) {
        return;
    }
    // 1/2 概率来点小动作
    if ((esp_random() % 2) == 0) {
        return;
    }
    switch (esp_random() % 3) {
    case 0:
        anim_once(&g_stretch);
        break;
    case 1:
        anim_once(&g_look);
        break;
    default:
        anim_once(&g_hum);
        break;
    }
}

// ---------------- 状态条与信息刷新 ----------------
static void bars_refresh(void)
{
    if (!s_state || !s_bar_mood) {
        return;
    }
    lv_obj_set_width(s_bar_mood, 170 * s_state->mood / PET_STAT_MAX);
    lv_obj_set_width(s_bar_full, 170 * s_state->fullness / PET_STAT_MAX);
    lv_obj_set_width(s_bar_aff, 170 * s_state->affection / PET_STAT_MAX);
}

static void about_refresh(void)
{
    if (!s_state || !s_about_level) {
        return;
    }
    lv_label_set_text_fmt(s_about_level, "Lv %d", pet_state_level(s_state));
    lv_label_set_text_fmt(s_about_count, "%lu 次",
                          (unsigned long)s_state->total_interactions);
}

void pet_ui_battery(int soc)
{
    if (!s_battery_label) {
        return;
    }
    if (soc < 0) {
        lv_label_set_text(s_battery_label, "--");
    } else if (soc > 100) {
        lv_label_set_text(s_battery_label, "100%");
    } else {
        lv_label_set_text_fmt(s_battery_label, "%d%%", soc);
    }
}

// ---------------- 页面构建 ----------------
static void make_top_bar(lv_obj_t *scr)
{
    lv_obj_t *plate = block(scr, 12, 10, 96, 30, C_INK);
    lv_obj_set_style_radius(plate, 10, 0);
    lv_obj_t *title = label(scr, 0, 0, "小鲸", &app_font_20, C_CARD);
    lv_obj_align_to(title, plate, LV_ALIGN_CENTER, 0, 0);
}

static void make_battery(lv_obj_t *scr)
{
    lv_obj_t *box = block(scr, 152, 12, 76, 28, C_CARD);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(C_INK), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    s_battery_label = label(scr, 0, 0, "--", &app_font_16, C_INK);
    lv_obj_align_to(s_battery_label, box, LV_ALIGN_CENTER, 0, 0);
}

static void make_bubble(lv_obj_t *scr)
{
    lv_obj_t *b = block(scr, 40, 44, 160, 28, C_CARD);
    lv_obj_set_style_radius(b, 14, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(C_INK), 0);
    lv_obj_set_style_border_width(b, 2, 0);
    s_bubble = lv_label_create(scr);
    lv_obj_set_pos(s_bubble, 46, 46);
    lv_obj_set_size(s_bubble, 148, 24);
    lv_label_set_long_mode(s_bubble, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(s_bubble, &app_font_16, 0);
    lv_obj_set_style_text_color(s_bubble, lv_color_hex(C_INK), 0);
    lv_obj_set_style_text_align(s_bubble, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_bubble, "");
}

static void make_card(lv_obj_t *scr)
{
    lv_obj_t *card = block(scr, 34, 80, 172, 168, C_CARD);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(C_INK), 0);
    lv_obj_set_style_border_width(card, 3, 0);
    s_img = lv_image_create(scr);
    lv_obj_set_pos(s_img, 40, 84);
    lv_image_set_src(s_img, &s_f_idle[0]);
}

static void make_bar(lv_obj_t *scr, int y, const char *text, uint32_t color, lv_obj_t **fill)
{
    lv_obj_t *l = label(scr, 8, y - 6, text, &app_font_16, C_INK);
    lv_obj_set_width(l, 46);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *slot = block(scr, 58, y, 170, 10, C_SLOT);
    lv_obj_set_style_radius(slot, 5, 0);
    *fill = block(scr, 58, y, 0, 10, color);
    lv_obj_set_style_radius(*fill, 5, 0);
}

// 三个按键小按钮卡片(借鉴桌宠插件的按钮布局)
static void make_key_buttons(lv_obj_t *scr)
{
    static const struct {
        int x;
        uint32_t color;
        const char *text;
    } btns[] = {
        {14, C_FULL, "喂食"},
        {88, C_MOOD, "摸摸"},
        {162, C_AFF, "玩耍"},
    };
    for (size_t i = 0; i < sizeof(btns) / sizeof(btns[0]); i++) {
        lv_obj_t *b = block(scr, btns[i].x, 298, 64, 20, btns[i].color);
        lv_obj_set_style_radius(b, 10, 0);
        lv_obj_t *t = label(scr, 0, 0, btns[i].text, &app_font_16, C_CARD);
        lv_obj_align_to(t, b, LV_ALIGN_CENTER, 0, 0);
    }
}

static void build_main_screen(void)
{
    s_main_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_main_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_main_scr, lv_color_hex(C_BG), 0);
    lv_obj_set_style_border_width(s_main_scr, 0, 0);
    lv_obj_set_style_pad_all(s_main_scr, 0, 0);

    make_top_bar(s_main_scr);
    make_battery(s_main_scr);
    make_bubble(s_main_scr);
    make_card(s_main_scr);
    make_bar(s_main_scr, 256, "心情", C_MOOD, &s_bar_mood);
    make_bar(s_main_scr, 272, "饱食", C_FULL, &s_bar_full);
    make_bar(s_main_scr, 288, "好感", C_AFF, &s_bar_aff);
    make_key_buttons(s_main_scr);
}

static void build_about_screen(void)
{
    s_about_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_about_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_about_scr, lv_color_hex(C_BG), 0);
    lv_obj_set_style_border_width(s_about_scr, 0, 0);
    lv_obj_set_style_pad_all(s_about_scr, 0, 0);

    lv_obj_t *plate = block(s_about_scr, 12, 14, 132, 32, C_INK);
    lv_obj_set_style_radius(plate, 10, 0);
    lv_obj_t *title = label(s_about_scr, 0, 0, "关于小鲸", &app_font_20, C_CARD);
    lv_obj_align_to(title, plate, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *mini_card = block(s_about_scr, 14, 66, 96, 96, C_CARD);
    lv_obj_set_style_radius(mini_card, 14, 0);
    lv_obj_set_style_border_color(mini_card, lv_color_hex(C_INK), 0);
    lv_obj_set_style_border_width(mini_card, 2, 0);
    lv_obj_t *mini = lv_image_create(s_about_scr);
    lv_obj_set_pos(mini, 22, 74);
    lv_image_set_src(mini, &s_f_about);
    lv_image_set_scale(mini, 128);

    lv_obj_t *panel = block(s_about_scr, 122, 66, 104, 130, C_CARD);
    lv_obj_set_style_radius(panel, 14, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(C_INK), 0);
    lv_obj_set_style_border_width(panel, 2, 0);

    label(s_about_scr, 134, 78, "等级", &app_font_16, C_INK);
    s_about_level = label(s_about_scr, 134, 98, "Lv 1", &app_font_20, C_AFF);
    label(s_about_scr, 134, 132, "互动", &app_font_16, C_INK);
    s_about_count = label(s_about_scr, 134, 152, "0 次", &app_font_20, C_INK);
    label(s_about_scr, 134, 186, "版本", &app_font_16, C_INK);

    lv_obj_t *ver = lv_label_create(s_about_scr);
    lv_obj_set_pos(ver, 134, 206);
    lv_obj_set_size(ver, 84, 20);
    lv_label_set_long_mode(ver, LV_LABEL_LONG_DOT);
    lv_label_set_text(ver, s_fw_version);
    lv_obj_set_style_text_font(ver, &app_font_16, 0);
    lv_obj_set_style_text_color(ver, lv_color_hex(C_INK), 0);

    lv_obj_t *hint = label(s_about_scr, 0, 296, "长按OK返回", &app_font_16, C_INK);
    lv_obj_set_width(hint, 240);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
}

// ---------------- 对外接口 ----------------
void pet_ui_init(pet_state_t *state, const char *fw_version)
{
    s_state = state;
    snprintf(s_fw_version, sizeof(s_fw_version), "%s",
             fw_version && fw_version[0] ? fw_version : "?");

    build_main_screen();
    build_about_screen();
    lv_screen_load(s_main_scr);

    bars_refresh();
    about_refresh();
    pet_ui_battery(bsp_battery_soc());
    bubble_show(LINE_WELCOME);

    s_asleep = false;
    s_once = NULL;
    anim_set_loop(&g_idle);

    s_anim_timer = lv_timer_create(anim_tick_cb, 250, NULL);
    s_bubble_timer = lv_timer_create(bubble_clear_cb, 8000, NULL);
    s_minute_timer = lv_timer_create(minute_cb, 60000, NULL);
    s_battery_timer = lv_timer_create(battery_cb, 30000, NULL);
    s_idle_timer = lv_timer_create(idle_act_cb, 20000, NULL);
}

static void battery_cb(lv_timer_t *timer)
{
    (void)timer;
    pet_ui_battery(bsp_battery_soc());
}

static void minute_cb(lv_timer_t *timer)
{
    (void)timer;
    pet_state_advance(s_state, 1);
    bars_refresh();
    about_refresh();

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    // 空闲 5 分钟自动入睡
    if (!s_asleep && now_ms - s_state->last_action_ms >= PET_SLEEPY_AFTER_MS) {
        pet_fall_asleep();
        s_save_pending = true;
        return;
    }
    if (s_asleep) {
        return;
    }
    pet_bubble_t kind = pet_state_pick_bubble(s_state, now_ms);
    if (kind != PET_BUBBLE_NONE && kind != PET_BUBBLE_SLEEPY) {
        bubble_show_state_lines();
    } else {
        const char *cur = lv_label_get_text(s_bubble);
        if (cur == NULL || cur[0] == '\0') {
            bubble_show(pick(LINES_IDLE, sizeof(LINES_IDLE) / sizeof(LINES_IDLE[0])));
        }
    }
    s_save_pending = true;
}

void pet_ui_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (!bsp_lvgl_lock(200)) {
        return;
    }

    if (ev == BSP_BTN_LONG && btn == BSP_BTN_OK) {
        if (lv_screen_active() == s_about_scr) {
            lv_screen_load(s_main_scr);
        } else {
            about_refresh();
            lv_screen_load(s_about_scr);
        }
        bsp_lvgl_unlock();
        return;
    }

    if (ev != BSP_BTN_CLICK) {
        bsp_lvgl_unlock();
        return;
    }
    if (lv_screen_active() == s_about_scr) {
        bsp_lvgl_unlock();
        return;
    }

    // 睡着时任意按键 = 叫醒(不执行原动作)
    if (s_asleep) {
        pet_wake_up();
        bsp_lvgl_unlock();
        return;
    }

    pet_action_t action;
    const anim_group_t *anim;
    const char *line;
    switch (btn) {
    case BSP_BTN_UP:
        action = PET_ACTION_FEED;
        anim = &g_eat;
        line = pick(LINES_FEED, sizeof(LINES_FEED) / sizeof(LINES_FEED[0]));
        break;
    case BSP_BTN_DOWN:
        action = PET_ACTION_PLAY;
        anim = &g_play;
        line = pick(LINES_PLAY, sizeof(LINES_PLAY) / sizeof(LINES_PLAY[0]));
        break;
    case BSP_BTN_OK:
    default:
        action = PET_ACTION_PET;
        // 摸摸随机三反应(借鉴桌宠插件"点击回应 ×3")
        switch (esp_random() % 3) {
        case 0:
            anim = &g_happy;
            line = pick(LINES_HAPPY, sizeof(LINES_HAPPY) / sizeof(LINES_HAPPY[0]));
            break;
        case 1:
            anim = &g_shy;
            line = pick(LINES_SHY, sizeof(LINES_SHY) / sizeof(LINES_SHY[0]));
            break;
        default:
            anim = &g_tsun;
            line = pick(LINES_TSUN, sizeof(LINES_TSUN) / sizeof(LINES_TSUN[0]));
            break;
        }
        break;
    }

    int old_level = pet_state_level(s_state);
    bool applied = pet_state_apply(s_state, action,
                                   (uint32_t)(esp_timer_get_time() / 1000));
    if (!applied) {
        bsp_lvgl_unlock();
        return;
    }

    anim_once(anim);
    bars_refresh();
    if (pet_state_level(s_state) > old_level) {
        bubble_show(LINE_LEVELUP);
    } else {
        bubble_show(line);
    }
    s_save_pending = true;
    bsp_lvgl_unlock();
}

bool pet_ui_minute_tick(void)
{
    bool pending = s_save_pending;
    s_save_pending = false;
    return pending;
}

void pet_ui_deinit(void)
{
    if (s_anim_timer) {
        lv_timer_delete(s_anim_timer);
        s_anim_timer = NULL;
    }
    if (s_bubble_timer) {
        lv_timer_delete(s_bubble_timer);
        s_bubble_timer = NULL;
    }
    if (s_minute_timer) {
        lv_timer_delete(s_minute_timer);
        s_minute_timer = NULL;
    }
    if (s_battery_timer) {
        lv_timer_delete(s_battery_timer);
        s_battery_timer = NULL;
    }
    if (s_idle_timer) {
        lv_timer_delete(s_idle_timer);
        s_idle_timer = NULL;
    }
    if (s_main_scr) {
        lv_obj_delete(s_main_scr);
        s_main_scr = NULL;
    }
    if (s_about_scr) {
        lv_obj_delete(s_about_scr);
        s_about_scr = NULL;
    }
    s_img = NULL;
    s_bubble = NULL;
    s_battery_label = NULL;
    s_bar_mood = NULL;
    s_bar_full = NULL;
    s_bar_aff = NULL;
    s_about_level = NULL;
    s_about_count = NULL;
}
