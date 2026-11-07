// main/pet_ui.h —— 「小鲸」的 LVGL 界面:主页、动画、气泡与状态页。
#pragma once

#include "bsp_button.h"
#include "pet_state.h"

// 初始化并加载主页面。需在 LVGL 就绪、持 bsp_lvgl_lock 时调用。
void pet_ui_init(pet_state_t *state, const char *fw_version);

// 按键分发(在 input 任务里调用,内部自行持锁)。
void pet_ui_key(bsp_btn_t btn, bsp_btn_ev_t ev);

// 分钟 tick(LVGL 定时器上下文):推进衰减、刷新状态、重选气泡、触发 NVS 保存标记。
// 返回 true 表示状态发生变化(调用方据此标记持久化)。
bool pet_ui_minute_tick(void);

// 电量刷新:-1 表示读数不可用。
void pet_ui_battery(int soc);

// 反初始化:停掉本页创建的定时器并删除屏幕(LVGL 任务上下文,持锁调用)。
void pet_ui_deinit(void);
