// main/pet_state.c —— 纯逻辑实现,不依赖 ESP-IDF/LVGL。
#include "pet_state.h"

#define MAGIC_0 'W'
#define MAGIC_1 'H'
#define MAGIC_2 'L'
#define MAGIC_3 '1'

static int clamp100(int v)
{
    if (v < 0) {
        return 0;
    }
    if (v > PET_STAT_MAX) {
        return PET_STAT_MAX;
    }
    return v;
}

void pet_state_init(pet_state_t *s)
{
    s->mood = 80;
    s->fullness = 70;
    s->affection = 0;
    s->total_interactions = 0;
    s->last_action_ms = 0;
}

bool pet_state_apply(pet_state_t *s, pet_action_t action, uint32_t now_ms)
{
    if (now_ms - s->last_action_ms < PET_COOLDOWN_MS) {
        return false;
    }

    switch (action) {
    case PET_ACTION_PET:
        s->affection = clamp100(s->affection + 2);
        s->mood = clamp100(s->mood + 3);
        break;
    case PET_ACTION_FEED:
        s->fullness = clamp100(s->fullness + 15);
        s->mood = clamp100(s->mood + 2);
        break;
    case PET_ACTION_PLAY:
        s->mood = clamp100(s->mood + 10);
        s->fullness = clamp100(s->fullness - 5);
        break;
    }

    s->total_interactions++;
    s->last_action_ms = now_ms;
    return true;
}

void pet_state_advance(pet_state_t *s, uint32_t minutes)
{
    if (minutes == 0) {
        return;
    }
    s->mood = clamp100(s->mood - (int)minutes);
    s->fullness = clamp100(s->fullness - 2 * (int)minutes);
}

int pet_state_level(const pet_state_t *s)
{
    int level = 1 + (int)(s->total_interactions / 10);
    if (level > 9) {
        level = 9;
    }
    return level;
}

pet_bubble_t pet_state_pick_bubble(const pet_state_t *s, uint32_t now_ms)
{
    if (s->fullness < 30) {
        return PET_BUBBLE_HUNGRY;
    }
    if (now_ms - s->last_action_ms >= PET_SLEEPY_AFTER_MS) {
        return PET_BUBBLE_SLEEPY;
    }
    if (s->mood < 30) {
        return PET_BUBBLE_LOW_MOOD;
    }
    return PET_BUBBLE_NONE;
}

size_t pet_state_save(const pet_state_t *s, uint8_t *buf, size_t cap)
{
    if (cap < PET_BLOB_SIZE) {
        return 0;
    }
    buf[0] = MAGIC_0;
    buf[1] = MAGIC_1;
    buf[2] = MAGIC_2;
    buf[3] = MAGIC_3;
    buf[4] = (uint8_t)clamp100(s->mood);
    buf[5] = (uint8_t)clamp100(s->fullness);
    buf[6] = (uint8_t)clamp100(s->affection);
    buf[7] = 0;
    buf[8] = (uint8_t)(s->total_interactions & 0xFF);
    buf[9] = (uint8_t)((s->total_interactions >> 8) & 0xFF);
    buf[10] = (uint8_t)((s->total_interactions >> 16) & 0xFF);
    buf[11] = (uint8_t)((s->total_interactions >> 24) & 0xFF);
    return PET_BLOB_SIZE;
}

bool pet_state_load(pet_state_t *s, const uint8_t *buf, size_t len)
{
    if (len < PET_BLOB_SIZE) {
        return false;
    }
    if (buf[0] != MAGIC_0 || buf[1] != MAGIC_1 ||
        buf[2] != MAGIC_2 || buf[3] != MAGIC_3) {
        return false;
    }
    if (buf[4] > PET_STAT_MAX || buf[5] > PET_STAT_MAX || buf[6] > PET_STAT_MAX) {
        return false;
    }
    s->mood = buf[4];
    s->fullness = buf[5];
    s->affection = buf[6];
    s->total_interactions = (uint32_t)buf[8] |
                            ((uint32_t)buf[9] << 8) |
                            ((uint32_t)buf[10] << 16) |
                            ((uint32_t)buf[11] << 24);
    s->last_action_ms = 0;
    return true;
}
