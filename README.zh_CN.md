<p align="right">
  <a href="README.md">English</a> · <strong>简体中文</strong>
</p>

# 小鲸 · DeepSeek 鲸鱼娘宠物

[FoloToy AI Passport](https://ai-passport.folotoy.cn/) 的离线虚拟宠物玩法：
一位 DeepSeek 蓝配色的像素风鲸鱼娘，会吃饭、玩耍、撒娇、睡觉，还会对你的
按键做出各种反应。

基于开源的
[FoloToy AI Passport 固件基线](https://github.com/folotoy/ai-passport) 开发
（ESP32-C3 · 8 MB Flash · 无 PSRAM · 240×320 ST7789 · 三颗按键）。

## 功能

- **三键玩法**：⬆️ 喂食 · 🆗 摸摸（随机三种反应：开心 / 害羞 / 傲娇）· ⬇️ 玩耍。
- **会变化的数值**：心情 / 饱食 / 好感随时间衰减，并通过 NVS 断电保存。
- **睡眠系统**：空闲 5 分钟自动入睡，按任意键会被惊醒动画叫醒。
- **待机小动作**：伸懒腰、东张西望、哼歌。
- **状态页**（OK 长按）：等级、累计互动次数、固件版本。
- **中文界面**：由 `tools/pet_texts.txt` 生成的可复现 OFL 字体子集（16/20px）。

## 素材

33 帧立绘（160×160 RGB565）提取自 MIT 授权的
[dsh-whale-girl-pet](https://github.com/yanzwzz/dsh-whale-girl-pet) WebM 动画；
`tools/gen_whale_assets.py` 绘制的像素画原始版本保留在
`assets/images/whale_pet/`。

## 构建

ESP-IDF 5.5.3（ESP32-C3）。参见
[docs/development/engineering/environment-setup.zh_CN.md](docs/development/engineering/environment-setup.zh_CN.md)，
然后运行固件闸门：

```bash
./tools/validate.sh --firmware   # 产出 build/FoloToy-AI-Passport-full.bin
```

把合并镜像从 `0x0` 刷入设备。完整设计见
[docs/PRD_DEEPSEEK_WHALE_PET.zh_CN.md](docs/PRD_DEEPSEEK_WHALE_PET.zh_CN.md)。

## 许可证

MIT。衍生自 [FoloToy/ai-passport](https://github.com/folotoy/ai-passport)（MIT）；
立绘动画来自 [dsh-whale-girl-pet](https://github.com/yanzwzz/dsh-whale-girl-pet)（MIT）；
字体为 Noto Sans SC（SIL Open Font License 1.1）。
