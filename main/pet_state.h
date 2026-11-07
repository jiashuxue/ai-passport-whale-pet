// main/pet_state.h —— 「小鲸」虚拟宠物的纯逻辑状态机(不依赖 ESP-IDF/LVGL,可 host 测试)。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PET_STAT_MAX 100
#define PET_COOLDOWN_MS 300
#define PET_SLEEPY_AFTER_MS (5u * 60u * 1000u)

typedef enum {
    PET_ACTION_PET = 0,   // OK 短按:摸摸
    PET_ACTION_FEED,      // UP 短按:喂食
    PET_ACTION_PLAY,      // DOWN 短按:玩耍
} pet_action_t;

typedef enum {
    PET_BUBBLE_NONE = 0,  // 无特殊状态,UI 自行决定待机台词
    PET_BUBBLE_HUNGRY,    // 饱食 < 30
    PET_BUBBLE_SLEEPY,    // 空闲超过 10 分钟
    PET_BUBBLE_LOW_MOOD,  // 心情 < 30
} pet_bubble_t;

typedef struct {
    int mood;                     // 心情 0..100
    int fullness;                 // 饱食 0..100
    int affection;                // 好感 0..100
    uint32_t total_interactions;  // 累计互动次数
    uint32_t last_action_ms;      // 运行时:最近一次成功互动的时间戳
} pet_state_t;

// 初始状态:心情 80、饱食 70、好感 0。
void pet_state_init(pet_state_t *s);

// 尝试执行互动。冷却期(距上次成功互动不足 PET_COOLDOWN_MS)内返回 false 且不改状态;
// 成功返回 true。所有数值钳制在 [0, PET_STAT_MAX]。
bool pet_state_apply(pet_state_t *s, pet_action_t action, uint32_t now_ms);

// 推进 minutes 分钟的自然衰减:mood -1/分钟,fullness -2/分钟,下限 0。
void pet_state_advance(pet_state_t *s, uint32_t minutes);

// 等级 = 1 + 互动次数/10,上限 9。
int pet_state_level(const pet_state_t *s);

// 根据当前状态选择特殊气泡(优先级:hungry > sleepy > low_mood > none)。
pet_bubble_t pet_state_pick_bubble(const pet_state_t *s, uint32_t now_ms);

// ---------- NVS blob 序列化(小端,固定布局,带 magic 与版本) ----------
#define PET_BLOB_SIZE 12u

// 写入成功返回字节数,容量不足返回 0。
size_t pet_state_save(const pet_state_t *s, uint8_t *buf, size_t cap);

// 解析失败(长度/magic 不符)返回 false 且不改动 *s。
bool pet_state_load(pet_state_t *s, const uint8_t *buf, size_t len);
