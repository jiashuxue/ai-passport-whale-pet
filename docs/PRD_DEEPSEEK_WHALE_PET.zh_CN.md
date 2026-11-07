<p align="right">
  <a href="PRD_DEEPSEEK_WHALE_PET.md">English</a> · <strong>简体中文</strong>
</p>

# PRD：DeepSeek 鲸鱼娘宠物「小鲸」

## 1. 概述

FoloToy AI Passport 的离线虚拟宠物：一位 DeepSeek 蓝配色的像素风 Q 版鲸鱼娘，
头上有呆毛与两侧小鳍，穿水手服、系红领巾，身后露出鲸鱼尾巴。玩家用三颗实体
按键给她喂食、摸摸、陪玩。心情、饱食与好感随时间变化，并通过 NVS 断电保存。

## 2. 目标硬件

ESP32-C3 · 8 MB Flash · 无 PSRAM · 240x320 竖屏 ST7789P3（RGB565，BSP 会把画面
四角按 30px 圆角遮罩）· 三键共用一路 ADC 阶梯。

## 3. 画面与布局

主页面（所有内容避开四角 30px 圆角遮罩）：

- 顶栏：名牌「小鲸」（app_font_20），右上角电量百分比（CW2017 读数不可用时
  隐藏或弱化显示）。
- 中央：白色圆角卡片（约 172x172）内放 160x160 立绘。由 LVGL 定时器切换帧；
  每帧为 RGB565，通过 EMBED_FILES 存于 flash（无 alpha，白色卡片背景直接烘焙
  进帧里）。
- 卡片上方的对话气泡：1-2 行中文台词，按状态选择。
- 底部：三条带标签的状态条（心情 / 饱食 / 好感）与按键提示行
  （UP 喂食 | OK 摸摸 | DOWN 玩耍）。

状态页（OK 长按进入、OK 长按返回）：「关于小鲸」标题、等级、累计互动次数、
固件版本、返回提示。

## 4. 按键定义

| 输入 | 效果 |
| --- | --- |
| OK 短按 | 摸摸：好感 +2、心情 +3；开心动画；随机夸奖气泡。 |
| UP 短按 | 喂食：饱食 +15、心情 +2；吃播动画；气泡。 |
| DOWN 短按 | 玩耍：心情 +10、饱食 -5；跳跃动画；气泡。 |
| OK 长按 | 切换状态页。 |

所有互动共用 300ms 冷却。按键回调只入队事件；UI 仅在 LVGL 任务中、持
`bsp_lvgl_lock()` 访问。

## 5. 状态模型（纯 C，可 host 测试）

- `mood`、`fullness`、`affection`：0..100，每次变更都做钳制。
- 每 60 秒 tick：mood -1、fullness -2（下限 0）。
- `total_interactions` 计数器；`level = 1 + total_interactions / 10`（上限 9）。
- 气泡选择：fullness < 30 说饿；空闲 10 分钟说困；mood < 30 说低落；否则显示
  最近一次互动气泡或随机待机台词。
- NVS 持久化：命名空间 "whale"，打包成 blob 存储；写入节流为每 60 秒一次，
  或状态有变化且距上次写入超过 5 秒时写入。启动时尽力读取。

## 6. 素材

- `tools/gen_whale_assets.py` 在 40x40 网格上绘制 10 帧（idle/happy/eat/play/
  sleep 各 2 帧），最近邻放大到 160x160，输出 PNG 与 RGB565 二进制到
  `main/assets/`。可重跑复现。
- `tools/pet_texts.txt` 是中文 UI 文案的唯一清单；`tools/font_pipeline/`
  据此生成子集字体。
- 字体：OFL 授权 CJK 子集，16px 与 20px，bpp 4，不压缩，命名
  `app_font_16` / `app_font_20`。

## 7. 验收标准

- `tools/validate.sh` 完整闸门通过：仓库检查、host 测试、固件闸门产出
  `build/FoloToy-AI-Passport-full.bin`。
- Host 测试覆盖状态机（钳制、衰减、冷却、等级、序列化往返）。
- 真机：开机直进宠物页（无 demo 菜单）；三个按键产生文档所述反应；中文无空白
  渲染；断电重开状态保留；电量正常显示或优雅降级。

## 8. 第一版不做

音效/语音、Wi-Fi、BLE、低功耗睡眠，以及任何网络服务。
