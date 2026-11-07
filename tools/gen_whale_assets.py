#!/usr/bin/env python3
"""Generate 'Xiao Jing' DeepSeek whale-girl pixel sprite frames.

Reproducible asset pipeline:
  * draws a 10-frame chibi whale girl on a 40x40 grid,
  * upscales 4x (nearest) to 160x160 PNGs on a white card background,
  * writes a labeled preview sheet for human approval,
  * optionally emits raw RGB565 binaries for LVGL (`--emit-bin`).

Outputs:
  assets/images/whale_pet/frame_<name>.png
  assets/images/whale_pet/preview.png
  main/assets/whale_<name>.rgb565          (with --emit-bin)

Requires Pillow. Run from the repository root:
  python tools/gen_whale_assets.py [--emit-bin]
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "images" / "whale_pet"
GRID = 40
SCALE = 4
SIZE = GRID * SCALE  # 160

# ---------------------------------------------------------------- palette
HAIR_D = (22, 48, 124)     # deep outline blue
HAIR = (45, 90, 254)       # main DeepSeek blue
HAIR_L = (103, 140, 255)   # highlight
HAIR_T = (77, 107, 254)    # hair tip accent
INK = (23, 32, 42)
EYE = (26, 43, 76)
SKIN = (255, 227, 201)
SKIN_SH = (244, 197, 160)
BLUSH = (255, 158, 181)
MOUTH = (214, 106, 126)
WHITE = (255, 255, 255)
UNIFORM = (255, 255, 255)
UNIFORM_SH = (224, 232, 255)
SCARF = (228, 59, 47)
SPLASH = (127, 212, 255)
FISH = (255, 214, 165)
TRANSPARENT = (0, 0, 0, 0)


class Frame:
    def __init__(self) -> None:
        self.img = Image.new("RGBA", (GRID, GRID), TRANSPARENT)
        self.d = ImageDraw.Draw(self.img)

    def px(self, x: int, y: int, c, w: int = 1, h: int = 1) -> None:
        if 0 <= x < GRID and 0 <= y < GRID:
            self.d.rectangle([x, y, x + w - 1, y + h - 1], fill=c)

    def rect(self, x, y, w, h, c) -> None:
        self.px(x, y, c, w, h)


def draw_base(f: Frame, dy: int = 0) -> None:
    """Body shared by all frames; dy shifts the whole figure vertically."""
    y = dy
    # whale side fins on the hair
    f.px(7, 1 + y, HAIR_D); f.px(8, 1 + y, HAIR); f.px(9, 1 + y, HAIR_D)
    f.px(7, 2 + y, HAIR_D); f.px(8, 2 + y, HAIR); f.px(9, 2 + y, HAIR_D); f.px(10, 2 + y, HAIR_D)
    f.px(8, 3 + y, HAIR_D)
    f.px(31, 1 + y, HAIR_D); f.px(32, 1 + y, HAIR); f.px(33, 1 + y, HAIR_D)
    f.px(30, 2 + y, HAIR_D); f.px(31, 2 + y, HAIR); f.px(32, 2 + y, HAIR); f.px(33, 2 + y, HAIR_D)
    f.px(32, 3 + y, HAIR_D)
    # ahoge (single curled hair) + water droplet
    f.px(20, 0 + y, HAIR_D)
    f.px(19, 1 + y, HAIR_D); f.px(20, 1 + y, HAIR_L)
    f.px(20, 2 + y, HAIR)
    f.px(21, 3 + y, HAIR)
    f.px(4, 5 + y, SPLASH); f.px(5, 4 + y, SPLASH); f.px(5, 5 + y, SPLASH); f.px(5, 6 + y, SPLASH)
    f.px(6, 5 + y, SPLASH)
    # hair mass: outline then fill
    f.rect(8, 3 + y, 24, 19, HAIR_D)
    f.rect(9, 4 + y, 22, 17, HAIR)
    # side locks falling to the shoulders
    f.rect(7, 12 + y, 2, 21, HAIR_D)
    f.rect(8, 12 + y, 1, 20, HAIR)
    f.rect(31, 12 + y, 2, 21, HAIR_D)
    f.rect(31, 12 + y, 1, 20, HAIR)
    # hair shine dots
    for sx in (11, 15, 25):
        f.px(sx, 7 + y, HAIR_L)
    f.px(27, 9 + y, HAIR_L)
    # face
    f.rect(11, 12 + y, 18, 11, SKIN)
    # bangs: zig-zag fringe over the forehead
    for bx in range(10, 30):
        f.px(bx, 11 + y, HAIR_D)
        f.px(bx, 12 + y, HAIR)
    f.px(12, 13 + y, HAIR); f.px(13, 13 + y, HAIR)
    f.px(16, 13 + y, HAIR); f.px(17, 13 + y, HAIR)
    f.px(20, 13 + y, HAIR); f.px(21, 13 + y, HAIR)
    f.px(24, 13 + y, HAIR); f.px(25, 13 + y, HAIR)
    # neck
    f.rect(18, 23 + y, 4, 2, SKIN_SH)
    # sailor top
    f.rect(11, 24 + y, 18, 14, INK)
    f.rect(12, 25 + y, 16, 12, UNIFORM)
    f.rect(12, 25 + y, 16, 2, HAIR_D)          # collar band
    f.px(12, 25 + y, UNIFORM_SH); f.px(27, 25 + y, UNIFORM_SH)
    # V-neck skin triangle
    f.px(19, 27 + y, SKIN); f.px(20, 27 + y, SKIN)
    f.px(19, 28 + y, SKIN_SH); f.px(20, 28 + y, SKIN_SH)
    # red neckerchief
    f.px(18, 28 + y, SCARF); f.px(19, 28 + y, SCARF); f.px(20, 28 + y, SCARF); f.px(21, 28 + y, SCARF)
    f.px(18, 29 + y, SCARF); f.px(19, 29 + y, SCARF); f.px(20, 29 + y, SCARF); f.px(21, 29 + y, SCARF)
    f.px(20, 30 + y, SCARF)
    # arms
    f.rect(10, 27 + y, 2, 3, SKIN)
    f.rect(28, 27 + y, 2, 3, SKIN)
    # skirt bottom shading
    f.rect(12, 35 + y, 16, 2, UNIFORM_SH)
    # feet
    f.rect(13, 37 + y, 4, 1, INK)
    f.rect(23, 37 + y, 4, 1, INK)
    # whale tail peeking out behind (right side)
    f.px(30, 28 + y, HAIR_D); f.px(31, 28 + y, HAIR_D)
    f.px(31, 27 + y, HAIR); f.px(32, 27 + y, HAIR)
    f.px(32, 28 + y, HAIR); f.px(33, 28 + y, HAIR)
    f.px(32, 29 + y, HAIR_D); f.px(33, 29 + y, HAIR_D)
    f.px(31, 30 + y, HAIR); f.px(32, 30 + y, HAIR)
    f.px(30, 29 + y, HAIR_T); f.px(30, 30 + y, HAIR_T)


def eyes_open(f: Frame, dy: int) -> None:
    f.rect(14, 14 + dy, 3, 4, EYE)
    f.rect(23, 14 + dy, 3, 4, EYE)
    f.px(14, 14 + dy, WHITE)
    f.px(23, 14 + dy, WHITE)


def eyes_closed(f: Frame, dy: int) -> None:
    f.rect(14, 16 + dy, 3, 1, EYE)
    f.rect(23, 16 + dy, 3, 1, EYE)


def eyes_happy(f: Frame, dy: int) -> None:
    f.px(13, 15 + dy, EYE); f.px(14, 14 + dy, EYE); f.px(15, 15 + dy, EYE)
    f.px(25, 15 + dy, EYE); f.px(26, 14 + dy, EYE); f.px(27, 15 + dy, EYE)


def mouth_smile(f: Frame, dy: int) -> None:
    f.rect(19, 20 + dy, 2, 1, MOUTH)


def mouth_open(f: Frame, dy: int) -> None:
    f.rect(18, 19 + dy, 4, 3, MOUTH)
    f.px(19, 20 + dy, WHITE)


def blush(f: Frame, dy: int, big: bool = False) -> None:
    w = 3 if big else 2
    h = 2 if big else 2
    f.rect(11, 18 + dy, w, h, BLUSH)
    f.rect(29 - w, 18 + dy, w, h, BLUSH)


def make_idle0() -> Image.Image:
    f = Frame()
    draw_base(f)
    eyes_open(f, 0)
    mouth_smile(f, 0)
    blush(f, 0)
    return f.img


def make_idle1() -> Image.Image:
    f = Frame()
    draw_base(f)
    eyes_closed(f, 0)
    mouth_smile(f, 0)
    blush(f, 0)
    return f.img


def make_happy0() -> Image.Image:
    f = Frame()
    draw_base(f, dy=1)
    eyes_happy(f, 1)
    mouth_open(f, 1)
    blush(f, 1, big=True)
    return f.img


def make_happy1() -> Image.Image:
    f = Frame()
    draw_base(f, dy=-1)
    eyes_happy(f, -1)
    mouth_open(f, -1)
    blush(f, -1, big=True)
    return f.img


def make_eat0() -> Image.Image:
    f = Frame()
    draw_base(f)
    eyes_open(f, 0)
    mouth_open(f, 0)
    blush(f, 0)
    # little fish held up to the mouth
    f.px(24, 18, FISH); f.px(25, 18, FISH); f.px(26, 18, FISH)
    f.px(26, 19, HAIR_T); f.px(27, 18, HAIR_T); f.px(27, 19, HAIR_T)
    f.px(24, 19, INK)
    return f.img


def make_eat1() -> Image.Image:
    f = Frame()
    draw_base(f)
    eyes_happy(f, 0)
    mouth_smile(f, 0)
    f.rect(17, 19, 2, 2, BLUSH)
    f.rect(21, 19, 2, 2, BLUSH)
    return f.img


def make_play0() -> Image.Image:
    f = Frame()
    draw_base(f, dy=-2)
    eyes_happy(f, -2)
    mouth_open(f, -2)
    blush(f, -2)
    return f.img


def make_play1() -> Image.Image:
    f = Frame()
    draw_base(f, dy=2)
    eyes_open(f, 2)
    mouth_smile(f, 2)
    blush(f, 2)
    return f.img


def make_sleep0() -> Image.Image:
    f = Frame()
    draw_base(f)
    eyes_closed(f, 0)
    f.rect(19, 21, 2, 1, MOUTH)
    # zzz bubble
    f.px(30, 5, WHITE); f.px(31, 5, WHITE); f.px(30, 6, WHITE)
    f.px(33, 3, WHITE); f.px(34, 3, WHITE); f.px(33, 4, WHITE)
    return f.img


def make_sleep1() -> Image.Image:
    f = Frame()
    draw_base(f)
    eyes_closed(f, 0)
    f.rect(19, 21, 2, 1, MOUTH)
    f.px(31, 6, WHITE); f.px(32, 6, WHITE); f.px(31, 7, WHITE)
    f.px(34, 4, WHITE); f.px(35, 4, WHITE); f.px(34, 5, WHITE)
    return f.img


FRAMES = [
    ("idle_0", make_idle0),
    ("idle_1", make_idle1),
    ("happy_0", make_happy0),
    ("happy_1", make_happy1),
    ("eat_0", make_eat0),
    ("eat_1", make_eat1),
    ("play_0", make_play0),
    ("play_1", make_play1),
    ("sleep_0", make_sleep0),
    ("sleep_1", make_sleep1),
]


def make_preview(images: list[tuple[str, Image.Image]]) -> Image.Image:
    cols, rows = 5, 2
    cell = SIZE + 16
    sheet = Image.new("RGBA", (cols * cell + 8, rows * cell + 24), (24, 32, 56, 255))
    d = ImageDraw.Draw(sheet)
    for i, (name, img) in enumerate(images):
        x = 8 + (i % cols) * cell
        y = 8 + (i // cols) * cell
        bg = Image.new("RGBA", (SIZE, SIZE), (240, 248, 255, 255))
        bg.alpha_composite(img.resize((SIZE, SIZE), Image.NEAREST))
        sheet.alpha_composite(bg, (x, y))
        d.text((x, y + SIZE + 4), name, fill=(255, 255, 255, 255))
    return sheet


def rgb565(rgb: tuple) -> int:
    r, g, b = rgb[0], rgb[1], rgb[2]
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def emit_bin(frames: list[tuple[str, Image.Image]], out_dir: Path) -> None:
    """Write raw little-endian RGB565 binaries for EMBED_FILES."""
    out_dir.mkdir(parents=True, exist_ok=True)
    for name, img in frames:
        rgb = img.convert("RGB")
        data = bytearray()
        for y in range(SIZE):
            for x in range(SIZE):
                v = rgb565(rgb.getpixel((x, y)))
                data.append(v & 0xFF)
                data.append(v >> 8)
        out = out_dir / f"whale_{name}.rgb565"
        out.write_bytes(bytes(data))
        print(f"wrote {out.relative_to(ROOT)} ({len(data)} bytes)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emit-bin", action="store_true", help="also write .rgb565 binaries to main/assets/")
    args = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    images: list[tuple[str, Image.Image]] = []
    for name, maker in FRAMES:
        rgba = maker().resize((SIZE, SIZE), Image.NEAREST)
        # Composite onto a white card so the on-device RGB565 frames need no alpha.
        card = Image.new("RGBA", (SIZE, SIZE), (255, 255, 255, 255))
        card.alpha_composite(rgba)
        out = OUT_DIR / f"frame_{name}.png"
        card.save(out)
        images.append((name, card))
        print(f"wrote {out.relative_to(ROOT)}")

    preview = make_preview(images)
    preview_path = OUT_DIR / "preview.png"
    preview.save(preview_path)
    print(f"wrote {preview_path.relative_to(ROOT)}")

    if args.emit_bin:
        emit_bin(images, ROOT / "main" / "assets")
    return 0


if __name__ == "__main__":
    sys.exit(main())
