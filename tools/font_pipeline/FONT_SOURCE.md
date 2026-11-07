<p align="right">
  <a href="FONT_SOURCE.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Font Source and Provenance Record

This file records the source font, license, and conversion parameters for the
generated LVGL font subsets `assets/fonts/app_font_16.c` and
`assets/fonts/app_font_20.c`. It is the companion record to the reproducible
generator `tools/font_pipeline/gen_font.py`.

## Source font

- Family:        Noto Sans SC (Regular, static weight 400 instance)
- Name-table:    family "Noto Sans SC"; version string
                 "Version 2.004-H2;hotconv 1.0.118;makeotfexe 2.5.65603"
- File cached:   tools/font_pipeline/NotoSansSC_400Regular.ttf
- Size:          10,559,284 bytes
- SHA-256:       D45F67F0A7C0CA3F256950777CE6A61CC7CE5F9696D02900CBBAAC25F8AA7D16
- Obtained on:   2026-10-23

How it was obtained: downloaded the npm package
`@expo-google-fonts/noto-sans-sc@0.4.4` from registry.npmmirror.com (the
official Chinese mirror of registry.npmjs.org) and extracted
`package/400Regular/NotoSansSC_400Regular.ttf`. That package distributes the
Google Fonts build of Noto Sans SC (upstream: the notofonts/noto-cjk project,
Adobe-Source-Han derived CJK family).

Why not GitHub: at generation time this machine could not reach
github.com / raw.githubusercontent.com / cdn.jsdelivr.net directly, so the
primary URLs in the task brief were unavailable. The npm-registry mirror
channel was reachable and provides the same OFL-licensed font with a clean
distribution chain.

## License

SIL Open Font License 1.1 (OFL-1.1).

- Full license text cached at: tools/font_pipeline/NOTO_SANS_SC_LICENSE_OFL.txt
  (extracted from the `@fontsource/noto-sans-sc@5.2.5` npm package).
- Upstream license reference:
  https://github.com/notofonts/noto-cjk (LICENSE = SIL OFL 1.1)
  https://openfontlicense.org

The generated `.c` files contain only rasterized glyph bitmap subsets for the
project's character inventory, not the font software itself. The original TTF
is kept only under tools/font_pipeline/ as a build cache and is not placed in
assets/.

## Converter

- Tool:     lv_font_conv 1.5.3 (latest npm release; pinned by
            tools/font_pipeline/package-lock.json; installed under
            tools/font_pipeline/node_modules). Rasterizer: FreeType built to
            WASM and shipped inside the package.
- Node.js:  24.15.0 (D:\AI\NODE\node.exe at generation time).
- Options:  --bpp 4 --format lvgl --no-compress
            --lv-font-name app_font_16 / app_font_20 --lv-include lvgl.h
- Note:     the project's reference documentation reports that lv_font_conv
            1.5.x could emit corrupt glyph bitmaps under some Node.js
            versions. gen_font.py therefore performs an independent
            cross-rasterization check (Pillow/FreeType) on every run and
            fails loudly if glyph bitmaps do not match within tolerance.

## Character inventory

- Source:     tools/pet_texts.txt (lines starting with '#' are comments).
- Extraction: every non-ASCII code point on non-comment lines, plus printable
              ASCII 0x20-0x7E.
- Total:      185 code points = 95 ASCII (0x20-0x7E) + 90 non-ASCII
              (89 CJK ideographs, U+2026 ellipsis, U+FF01 fullwidth
              exclamation, U+FF0C fullwidth comma).

## Generated outputs (verified)

- assets/fonts/app_font_16.c — 122,248 bytes, 16 px, 4 bpp, uncompressed
  (SHA-256 108F0C2BFEC934796A88C98658C1BD03E226CFE26D7AF4EFD2456C646B221C55)
- assets/fonts/app_font_20.c — 166,234 bytes, 20 px, 4 bpp, uncompressed
  (SHA-256 F8D9FE686F0DD50447F5E470BDB580EA5F116EF6EFA449E0F106311A388320DA)

Both files are UTF-8 without BOM and use the LVGL 9.x `lv_font_fmt_txt`
layout (cmap FORMAT0_TINY for ASCII, SPARSE_TINY for the CJK points), the
same layout as the LVGL 9.5.0 built-in fonts shipped in this repository.

## Verification results (2026-10-23)

- Coverage: all 185 required code points resolve to a glyph with a non-empty
  bitmap in both sizes. Missing glyphs: none.
- Bitmap cross-check against an independent FreeType rasterization
  (Pillow 12.3.0, 106-glyph sample per size):
  - app_font_16: median IoU 0.91, min 0.50
  - app_font_20: median IoU 0.90, min 0.32
  The lowest scorers are thin/small glyphs (U+FF01 "!", U+002D "-", U+0069
  "i", U+0027 "'") whose outlines differ by one hinted pixel between the
  converter's autohinted rendering and Pillow's default rendering; no glyph
  failed the corruption threshold.
- Eye check: reconstructed bitmaps for U+5C0F, U+9CB8, U+5FC3, U+FF01, and
  U+2026 render correctly
  (PNG previews kept in tools/font_pipeline/preview/).
- Reproducibility: running gen_font.py twice yields byte-identical .c files.

## Regeneration

From tools/font_pipeline/ run:

    "C:\Users\Administrator\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe" gen_font.py

The script reads tools/pet_texts.txt, the cached TTF, and the pinned
lv_font_conv, regenerates both .c files, re-verifies coverage and bitmaps,
and writes tools/font_pipeline/last_run_report.json. Add or change UI text
only through tools/pet_texts.txt and rerun the generator.

## Device status

Bitmaps were validated offline only. On-device rendering remains to be
checked when the application is flashed (Device tests: NOT RUN).
