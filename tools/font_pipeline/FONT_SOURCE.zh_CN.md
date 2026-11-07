<p align="right">
  <a href="FONT_SOURCE.md">English</a> · <strong>简体中文</strong>
</p>

# 字体来源与溯源记录

本文件记录生成的 LVGL 子集字体 `assets/fonts/app_font_16.c` 与
`assets/fonts/app_font_20.c` 的源字体、许可证与转换参数，是可复现生成器
`tools/font_pipeline/gen_font.py` 的配套记录。

## 源字体

- 字族：        Noto Sans SC（Regular，静态 400 字重实例）
- name 表：     字族 "Noto Sans SC"；版本串
                "Version 2.004-H2;hotconv 1.0.118;makeotfexe 2.5.65603"
- 缓存文件：    tools/font_pipeline/NotoSansSC_400Regular.ttf
- 大小：        10,559,284 字节
- SHA-256：     D45F67F0A7C0CA3F256950777CE6A61CC7CE5F9696D02900CBBAAC25F8AA7D16
- 获取日期：    2026-10-23

获取方式：从 registry.npmmirror.com（registry.npmjs.org 的官方国内镜像）
下载 npm 包 `@expo-google-fonts/noto-sans-sc@0.4.4`，解出
`package/400Regular/NotoSansSC_400Regular.ttf`。该包分发的是 Google Fonts
构建的 Noto Sans SC（上游：notofonts/noto-cjk 项目，思源黑体衍生的 CJK 字族）。

为何不用 GitHub：生成时本机无法直连 github.com / raw.githubusercontent.com /
cdn.jsdelivr.net，任务简报中的主 URL 不可用。npm 镜像渠道可达，且提供同样
OFL 许可的字体与干净的发行链。

## 许可证

SIL Open Font License 1.1（OFL-1.1）。

- 许可证全文缓存于：tools/font_pipeline/NOTO_SANS_SC_LICENSE_OFL.txt
  （提取自 `@fontsource/noto-sans-sc@5.2.5` npm 包）。
- 上游许可参考：
  https://github.com/notofonts/noto-cjk（LICENSE = SIL OFL 1.1）
  https://openfontlicense.org

生成的 `.c` 文件只包含本项目字符清单的光栅化字形位图子集，并非字体软件本身。
原始 TTF 仅作为构建缓存保存在 tools/font_pipeline/ 下，不放进 assets/。

## 转换器

- 工具：     lv_font_conv 1.5.3（npm 最新版；由
             tools/font_pipeline/package-lock.json 锁定；安装在
             tools/font_pipeline/node_modules）。光栅化器：内置的 WASM
             版 FreeType。
- Node.js：  24.15.0（生成时使用 D:\AI\NODE\node.exe）。
- 选项：     --bpp 4 --format lvgl --no-compress
             --lv-font-name app_font_16 / app_font_20 --lv-include lvgl.h
- 备注：     项目参考文档提到 lv_font_conv 1.5.x 在部分 Node.js 版本下可能
             写出损坏的字形位图。因此 gen_font.py 每次运行都会做独立的对照
             光栅化校验（Pillow/FreeType），字形位图超出容差即大声报错。

## 字符清单

- 来源：     tools/pet_texts.txt（以 '#' 开头的行是注释）。
- 提取规则： 非注释行中的全部非 ASCII 码点，加可打印 ASCII 0x20-0x7E。
- 合计：     185 个码点 = 95 个 ASCII（0x20-0x7E）+ 90 个非 ASCII
             （89 个 CJK 汉字，U+2026 省略号，U+FF01 全角叹号，
             U+FF0C 全角逗号）。

## 生成产物（已验证）

- assets/fonts/app_font_16.c — 122,248 字节，16px，4bpp，不压缩
  （SHA-256 108F0C2BFEC934796A88C98658C1BD03E226CFE26D7AF4EFD2456C646B221C55）
- assets/fonts/app_font_20.c — 166,234 字节，20px，4bpp，不压缩
  （SHA-256 F8D9FE686F0DD50447F5E470BDB580EA5F116EF6EFA449E0F106311A388320DA）

两个文件均为 UTF-8 无 BOM，使用 LVGL 9.x 的 `lv_font_fmt_txt` 布局
（ASCII 用 cmap FORMAT0_TINY，CJK 码点用 SPARSE_TINY），与本仓库随附的
LVGL 9.5.0 内置字体同款布局。

## 验证结果（2026-10-23）

- 覆盖：两个字号下，全部 185 个必需码点都解析到非空字形位图。缺失字符：无。
- 位图与独立 FreeType 光栅化结果对照（Pillow 12.3.0，每字号抽样 106 个字形）：
  - app_font_16：IoU 中位数 0.91，最低 0.50
  - app_font_20：IoU 中位数 0.90，最低 0.32
  最低分是细笔画小字形（U+FF01 "!"、U+002D "-"、U+0069 "i"、U+0027 "'"），
  差异来自转换器自动 hinting 与 Pillow 默认渲染之间 1 像素的笔画偏移；
  没有字形触及损坏阈值。
- 目检：从 .c 重建的 U+5C0F、U+9CB8、U+5FC3、U+FF01、U+2026 位图渲染正确
  （PNG 预览保留在 tools/font_pipeline/preview/）。
- 可复现：连续运行 gen_font.py 两次产出的 .c 文件逐字节一致。

## 重新生成

在 tools/font_pipeline/ 下运行：

    "C:\Users\Administrator\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe" gen_font.py

脚本读取 tools/pet_texts.txt、缓存的 TTF 与锁定的 lv_font_conv，重新生成两个
.c 文件，再次校验覆盖与位图，并写出 tools/font_pipeline/last_run_report.json。
修改 UI 文案只能通过 tools/pet_texts.txt 并重跑生成器。

## 设备状态

位图仅做离线验证。烧录应用后仍需在真机上确认渲染效果
（Device tests: NOT RUN）。
