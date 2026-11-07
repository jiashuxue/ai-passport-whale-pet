// tests/test_pet_state.c —— 「小鲸」状态机 host 测试(纯逻辑,无需硬件)。
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "pet_state.h"

int main(void)
{
    pet_state_t s;
    uint8_t blob[PET_BLOB_SIZE];

    // ---- 初始状态 ----
    pet_state_init(&s);
    assert(s.mood == 80);
    assert(s.fullness == 70);
    assert(s.affection == 0);
    assert(s.total_interactions == 0);
    assert(pet_state_level(&s) == 1);

    // ---- 喂食:数值与钳制 ----
    assert(pet_state_apply(&s, PET_ACTION_FEED, 1000));   // fullness 85
    assert(s.fullness == 85);
    assert(s.mood == 82);
    assert(s.total_interactions == 1);
    for (int i = 0; i < 5; i++) {
        (void)pet_state_apply(&s, PET_ACTION_FEED, 2000 + (uint32_t)i * 400);
    }
    assert(s.fullness == PET_STAT_MAX);                   // 封顶 100

    // ---- 冷却:同窗口内的第二次互动被拒绝 ----
    pet_state_t s2;
    pet_state_init(&s2);
    assert(pet_state_apply(&s2, PET_ACTION_PET, 5000));
    assert(!pet_state_apply(&s2, PET_ACTION_PET, 5000 + PET_COOLDOWN_MS - 1));
    assert(s2.total_interactions == 1);
    assert(pet_state_apply(&s2, PET_ACTION_PET, 5000 + PET_COOLDOWN_MS));
    assert(s2.total_interactions == 2);

    // ---- 玩耍:心情上限与饱食下限 ----
    pet_state_t s3;
    pet_state_init(&s3);
    s3.mood = 95;
    assert(pet_state_apply(&s3, PET_ACTION_PLAY, 1000));
    assert(s3.mood == PET_STAT_MAX);
    assert(s3.fullness == 65);
    s3.fullness = 2;
    assert(pet_state_apply(&s3, PET_ACTION_PLAY, 2000));
    assert(s3.fullness == 0);                             // 下限 0

    // ---- 摸摸:好感增长 ----
    pet_state_t s4;
    pet_state_init(&s4);
    assert(pet_state_apply(&s4, PET_ACTION_PET, 1000));
    assert(s4.affection == 2);
    assert(s4.mood == 83);

    // ---- 自然衰减 ----
    pet_state_t s5;
    pet_state_init(&s5);
    pet_state_advance(&s5, 10);
    assert(s5.mood == 70);
    assert(s5.fullness == 50);
    pet_state_advance(&s5, 1000);                         // 长期衰减到 0
    assert(s5.mood == 0);
    assert(s5.fullness == 0);
    pet_state_advance(&s5, 0);                            // 0 分钟不动
    assert(s5.mood == 0);

    // ---- 等级 = 1 + 互动/10,上限 9 ----
    pet_state_t s6;
    pet_state_init(&s6);
    s6.total_interactions = 0;
    assert(pet_state_level(&s6) == 1);
    s6.total_interactions = 9;
    assert(pet_state_level(&s6) == 1);
    s6.total_interactions = 10;
    assert(pet_state_level(&s6) == 2);
    s6.total_interactions = 999;
    assert(pet_state_level(&s6) == 9);

    // ---- 气泡优先级:hungry > sleepy > low_mood > none ----
    pet_state_t s7;
    pet_state_init(&s7);
    s7.fullness = 10;
    s7.mood = 10;
    s7.last_action_ms = 0;                                // 满足 sleepy 条件
    assert(pet_state_pick_bubble(&s7, 20 * 60 * 1000) == PET_BUBBLE_HUNGRY);
    s7.fullness = 50;
    assert(pet_state_pick_bubble(&s7, 20 * 60 * 1000) == PET_BUBBLE_SLEEPY);
    s7.last_action_ms = 1000;
    assert(pet_state_pick_bubble(&s7, 2000) == PET_BUBBLE_LOW_MOOD);
    s7.mood = 60;
    assert(pet_state_pick_bubble(&s7, 2000) == PET_BUBBLE_NONE);

    // ---- 序列化往返 ----
    pet_state_t s8;
    pet_state_init(&s8);
    s8.mood = 42;
    s8.fullness = 87;
    s8.affection = 66;
    s8.total_interactions = 0x12345678u;
    size_t n = pet_state_save(&s8, blob, sizeof(blob));
    assert(n == PET_BLOB_SIZE);
    pet_state_t s9;
    pet_state_init(&s9);
    assert(pet_state_load(&s9, blob, n));
    assert(s9.mood == 42);
    assert(s9.fullness == 87);
    assert(s9.affection == 66);
    assert(s9.total_interactions == 0x12345678u);

    // ---- 损坏数据被拒绝 ----
    uint8_t bad[PET_BLOB_SIZE];
    memset(bad, 0xEE, sizeof(bad));
    pet_state_t s10;
    pet_state_init(&s10);
    assert(!pet_state_load(&s10, bad, sizeof(bad)));
    assert(s10.mood == 80);                               // 原状态不被改动
    assert(!pet_state_load(&s10, blob, PET_BLOB_SIZE - 1));  // 长度不足
    memcpy(bad, blob, PET_BLOB_SIZE);
    bad[4] = 200;                                         // mood 越界
    assert(!pet_state_load(&s10, bad, sizeof(bad)));

    // ---- 容量不足时 save 返回 0 ----
    assert(pet_state_save(&s8, blob, PET_BLOB_SIZE - 1) == 0);

    return 0;
}
