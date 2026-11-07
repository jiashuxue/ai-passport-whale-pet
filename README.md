<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Xiao Jing · DeepSeek Whale-Girl Pet

An offline virtual pet play for the [FoloToy AI Passport](https://ai-passport.folotoy.cn/):
a pixel-style whale girl in DeepSeek blue who eats, plays, pets, sleeps, and reacts
to your buttons.

Built on the open-source
[FoloToy AI Passport firmware baseline](https://github.com/folotoy/ai-passport)
(ESP32-C3 · 8 MB flash · no PSRAM · 240×320 ST7789 · three buttons).

## Features

- **Three-button play**: UP feed · OK pet (3 random reactions: happy / shy / tsundere) · DOWN play.
- **Stats that matter**: mood / fullness / affection decay over time and persist in NVS across power loss.
- **Sleep system**: after 5 idle minutes she falls asleep; any key wakes her with a startle animation.
- **Idle antics**: stretching, looking around, or humming while idle.
- **Status page** (OK long press): level, total interactions, firmware version.
- **Chinese UI** with a reproducible OFL font subset (16/20 px) generated from `tools/pet_texts.txt`.

## Art

33 sprite frames (160×160 RGB565) extracted from the MIT-licensed
[dsh-whale-girl-pet](https://github.com/yanzwzz/dsh-whale-girl-pet) WebM animations;
the pixel-art originals drawn by `tools/gen_whale_assets.py` are kept under
`assets/images/whale_pet/`.

## Build

ESP-IDF 5.5.3 for ESP32-C3. See
[docs/development/engineering/environment-setup.md](docs/development/engineering/environment-setup.md),
then run the firmware gate:

```bash
./tools/validate.sh --firmware   # produces build/FoloToy-AI-Passport-full.bin
```

Flash the merged image at offset `0x0`. See
[docs/PRD_DEEPSEEK_WHALE_PET.md](docs/PRD_DEEPSEEK_WHALE_PET.md) for the full design.

## License

MIT. Derived from [FoloToy/ai-passport](https://github.com/folotoy/ai-passport) (MIT);
sprite animations from [dsh-whale-girl-pet](https://github.com/yanzwzz/dsh-whale-girl-pet) (MIT);
fonts are Noto Sans SC under the SIL Open Font License 1.1.
