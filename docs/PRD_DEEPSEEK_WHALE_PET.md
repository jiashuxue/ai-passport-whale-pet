<p align="right">
  <a href="PRD_DEEPSEEK_WHALE_PET.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# PRD: DeepSeek Whale-Girl Pet "Xiao Jing"

## 1. Summary

An offline virtual pet for the FoloToy AI Passport: a pixel-art chibi whale girl
in DeepSeek blue, wearing a sailor top with a red neckerchief, an ahoge, side
fins, and a whale tail. The player feeds, pets, and plays with her through the
three physical buttons. Mood, fullness, and affection evolve over time and
survive power loss through NVS.

## 2. Target

ESP32-C3 · 8 MB flash · no PSRAM · 240x320 ST7789P3 portrait (RGB565, corners
masked with 30 px radius by the BSP) · three buttons on one ADC ladder.

## 3. Screens and layout

Main screen (all corners kept clear of the 30 px radius mask):

- Top bar: title plate "Xiao Jing" (app_font_20), battery percentage top-right
  (hidden or dimmed when the CW2017 reading is unavailable).
- Center: white rounded card (~172x172) containing the 160x160 sprite. Frame
  sequence is switched by an LVGL timer; every frame is RGB565 stored in flash
  via EMBED_FILES (no alpha, white card background baked into the frames).
- Speech bubble above the card: 1-2 lines of Chinese text, selected by state.
- Bottom: three labeled stat bars (mood / fullness / affection) and a key-hint
  row (UP feed | OK pet | DOWN play).

Status page (OK long press enters, OK long press returns): title "About Xiao
Jing", level, total interactions, firmware version, and a return hint.

## 4. Button map

| Input | Effect |
| --- | --- |
| OK short | Pet: affection +2, mood +3; happy animation; random praise bubble. |
| UP short | Feed: fullness +15, mood +2; eat animation; bubble. |
| DOWN short | Play: mood +10, fullness -5; jump animation; bubble. |
| OK long | Toggle status page. |

All interactions share a 300 ms cooldown. Button callbacks only enqueue events;
the UI is only touched from the LVGL task under `bsp_lvgl_lock()`.

## 5. State model (pure C, host-tested)

- `mood`, `fullness`, `affection`: 0..100, clamped on every transition.
- Tick every 60 s: mood -1, fullness -2 (floor 0).
- `total_interactions` counter; `level = 1 + total_interactions / 10` (cap 9).
- Bubble selection: hungry when fullness < 30, sleepy after 10 min idle,
  low mood when mood < 30, otherwise the last interaction's bubble or an idle
  line.
- NVS persistence: namespace "whale", keys stored as a packed blob; writes are
  throttled to every 60 s or when the state changed and the last write is older
  than 5 s. Reads are best-effort at boot.

## 6. Assets

- `tools/gen_whale_assets.py` draws 10 frames (idle/happy/eat/play/sleep x2) on
  a 40x40 grid, upscales to 160x160, and writes PNGs plus raw RGB565 binaries
  into `main/assets/`. Reproducible: rerun to regenerate.
- `tools/pet_texts.txt` is the single inventory of Chinese UI strings;
  `tools/font_pipeline/` generates the subset fonts from it.
- Fonts: OFL-licensed CJK subset, 16 px and 20 px, bpp 4, uncompressed, named
  `app_font_16` / `app_font_20`.

## 7. Acceptance criteria

- `tools/validate.sh` complete gate passes: repository checks, host tests, and
  the firmware gate producing `build/FoloToy-AI-Passport-full.bin`.
- Host tests cover the state machine (clamps, decay, cooldown, level,
  serialization round-trip).
- On-device: boots straight into the pet screen (no demo menu), all three
  buttons produce the documented reaction, Chinese text renders without blanks,
  state survives a power cycle, battery value shows or degrades gracefully.

## 8. Out of scope (v1)

Audio/SFX, Wi-Fi, BLE, low-power sleep, and any network service.
