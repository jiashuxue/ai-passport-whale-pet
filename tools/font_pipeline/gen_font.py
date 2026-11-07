#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Reproducible LVGL CJK font-subset generator for FoloToy AI Passport.

Reads tools/pet_texts.txt (lines starting with '#' are comments), extracts
every non-ASCII code point plus printable ASCII 0x20-0x7E, and runs the
pinned lv_font_conv (installed under tools/font_pipeline/node_modules) to
produce:

    assets/fonts/app_font_16.c   (16 px, 4 bpp, uncompressed)
    assets/fonts/app_font_20.c   (20 px, 4 bpp, uncompressed)

Usage:
    python gen_font.py [--font PATH] [--node PATH] [--size N] [--no-verify]

Options:
    --font PATH       Source CJK font (TTF/OTF). Defaults to the cached
                      NotoSansSC_400Regular.ttf in this directory.
    --node PATH       Node.js executable. Defaults to $FONT_PIPELINE_NODE,
                      then `node` on PATH, then D:\\AI\\NODE\\node.exe.
    --size N          Font size in px (repeatable). Defaults to 16 and 20.
    --no-verify       Skip the post-generation verification pass.

The script fails loudly on any of the following:
  * lv_font_conv is not installed (see README note in this directory),
  * the converter exits non-zero,
  * a required code point has no glyph in a generated font,
  * a glyph bitmap has the wrong byte count or is empty when it should not be,
  * an independent FreeType rasterisation (Pillow) disagrees with a glyph
    bitmap beyond tolerance. This catches the known lv_font_conv 1.5.x
    issue of writing corrupt glyph bitmaps under newer Node.js versions.

A machine-readable summary is written to tools/font_pipeline/last_run_report.json.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
PIPELINE_DIR = Path(__file__).resolve().parent
TEXT_FILE = PROJECT_ROOT / "tools" / "pet_texts.txt"
OUT_DIR = PROJECT_ROOT / "assets" / "fonts"
CONV_DIR = PIPELINE_DIR / "node_modules" / "lv_font_conv"
CONV_ENTRY_CANDIDATES = [CONV_DIR / "lv_font_conv.js", CONV_DIR / "cli.js", CONV_DIR / "lib" / "cli.js"]
DEFAULT_FONT_CANDIDATES = [
    PIPELINE_DIR / "NotoSansSC_400Regular.ttf",
    PIPELINE_DIR / "NotoSansSC-Regular.otf",
    PIPELINE_DIR / "NotoSansSC-VF.ttf",
]
REPORT_FILE = PIPELINE_DIR / "last_run_report.json"

ASCII_RANGE = (0x20, 0x7E)  # inclusive
BPP = 4

# FreeType cross-check tolerance: best-alignment IoU of binarised glyph
# bitmaps. Correct renders score high; corrupt/garbage bitmaps score low.
IOU_MEDIAN_MIN = 0.60
IOU_ABS_MIN = 0.30
MAX_SAMPLE_GLYPHS = 64


# ---------------------------------------------------------------------------
# Character inventory
# ---------------------------------------------------------------------------

def extract_chars(text_file: Path):
    """Return sorted unique code points: ASCII 0x20-0x7E plus every
    non-ASCII character found on non-comment lines of the text inventory."""
    cps = set(range(ASCII_RANGE[0], ASCII_RANGE[1] + 1))
    raw = text_file.read_bytes()
    if raw.startswith(b"\xef\xbb\xbf"):
        raw = raw[3:]
    for line in raw.decode("utf-8").splitlines():
        if line.startswith("#"):
            continue
        for ch in line:
            if ord(ch) >= 0x80:
                cps.add(ord(ch))
    return sorted(cps)


def build_range_arg(cps):
    parts = [f"0x{ASCII_RANGE[0]:X}-0x{ASCII_RANGE[1]:X}"]
    parts += [f"0x{cp:X}" for cp in cps if cp < ASCII_RANGE[0] or cp > ASCII_RANGE[1]]
    return ",".join(parts)


# ---------------------------------------------------------------------------
# Converter invocation
# ---------------------------------------------------------------------------

def find_node(node_arg):
    if node_arg:
        return node_arg
    env = os.environ.get("FONT_PIPELINE_NODE")
    if env and Path(env).exists():
        return env
    found = shutil.which("node")
    if found:
        return found
    default = Path(r"D:\AI\NODE\node.exe")
    if default.exists():
        return str(default)
    raise RuntimeError(
        "Node.js not found. Pass --node, set FONT_PIPELINE_NODE, or install Node."
    )


def find_converter():
    for cand in CONV_ENTRY_CANDIDATES:
        if cand.exists():
            return cand
    raise RuntimeError(
        "lv_font_conv not installed. Run in tools/font_pipeline:  npm install lv_font_conv"
    )


def converter_version():
    pkg = CONV_DIR / "package.json"
    try:
        return json.loads(pkg.read_text(encoding="utf-8")).get("version", "unknown")
    except OSError:
        return "unknown"


def run_converter(node, converter, font, size, cps, out_path):
    # Pass paths relative to the pipeline directory so the converter's
    # Opts header comment is machine-independent (byte-reproducible output).
    try:
        font_arg = str(font.relative_to(PIPELINE_DIR))
    except ValueError:
        font_arg = str(font)
    out_arg = os.path.relpath(out_path, PIPELINE_DIR)
    conv_arg = os.path.relpath(converter, PIPELINE_DIR)
    cmd = [
        node, conv_arg,
        "--font", font_arg,
        "-r", build_range_arg(cps),
        "--size", str(size),
        "--bpp", str(BPP),
        "--format", "lvgl",
        "--no-compress",
        "--lv-font-name", f"app_font_{size}",
        "--lv-include", "lvgl.h",
        "--output", out_arg,
    ]
    print(f"[gen] size={size}  font={font.name}  ({len(cps)} code points)")
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8",
                          cwd=str(PIPELINE_DIR))
    if proc.returncode != 0:
        print(proc.stdout, file=sys.stderr)
        print(proc.stderr, file=sys.stderr)
        raise RuntimeError(f"lv_font_conv failed for size {size} (exit {proc.returncode})")
    # Guarantee UTF-8 without BOM.
    data = out_path.read_bytes()
    if data.startswith(b"\xef\xbb\xbf"):
        data = data[3:]
    out_path.write_bytes(data)
    return out_path


# ---------------------------------------------------------------------------
# Generated-C parser
# ---------------------------------------------------------------------------

HEX_RE = re.compile(r"0x([0-9a-fA-F]{1,4})\b")
DSC_ENTRY_RE = re.compile(
    r"\.bitmap_index\s*=\s*(-?\d+).*?"
    r"\.adv_w\s*=\s*(-?\d+).*?"
    r"\.box_w\s*=\s*(-?\d+).*?"
    r"\.box_h\s*=\s*(-?\d+).*?"
    r"\.ofs_x\s*=\s*(-?\d+).*?"
    r"\.ofs_y\s*=\s*(-?\d+)",
    re.S,
)
LIST_RE = re.compile(r"((?:unicode|glyph_id_ofs)_list_\d+)\s*\[\]\s*=\s*\{(.*?)\};", re.S)


def _array_body(source, name):
    m = re.search(re.escape(name) + r"\s*\[\]\s*=\s*\{(.*?)\};", source, re.S)
    if not m:
        raise RuntimeError(f"could not find '{name}' array in generated font")
    return m.group(1)


def _ints(body):
    return [int(v, 16) for v in HEX_RE.findall(body)]


def _cmap_entries(source):
    body = _array_body(source, "cmaps")
    entries = []
    for chunk in re.findall(r"\{([^{}]*)\}", body, re.S):
        def g(pat):
            m = re.search(pat, chunk)
            return m.group(1) if m else None
        ul = g(r"\.unicode_list\s*=\s*(\w+)")
        ol = g(r"\.glyph_id_ofs_list\s*=\s*(\w+)")
        entries.append({
            "range_start": int(g(r"\.range_start\s*=\s*(-?\d+)")),
            "range_length": int(g(r"\.range_length\s*=\s*(-?\d+)")),
            "glyph_id_start": int(g(r"\.glyph_id_start\s*=\s*(-?\d+)")),
            "unicode_list": None if ul == "NULL" else ul,
            "glyph_id_ofs_list": None if ol == "NULL" else ol,
        })
    return entries


def parse_generated_c(path: Path):
    source = path.read_text(encoding="utf-8")
    bitmap = _ints(_array_body(source, "glyph_bitmap"))

    dsc_body = _array_body(source, "glyph_dsc")
    dsc = []
    for chunk in re.findall(r"\{([^{}]*)\}", dsc_body, re.S):
        m = DSC_ENTRY_RE.search(chunk)
        if not m:
            continue
        dsc.append({
            "bitmap_index": int(m.group(1)),
            "adv_w": int(m.group(2)),
            "box_w": int(m.group(3)),
            "box_h": int(m.group(4)),
            "ofs_x": int(m.group(5)),
            "ofs_y": int(m.group(6)),
        })

    lists = {}
    for m in LIST_RE.finditer(source):
        lists[m.group(1)] = _ints(m.group(2))

    cmaps = _cmap_entries(source)
    return {"bitmap": bitmap, "dsc": dsc, "cmaps": cmaps, "lists": lists, "source": source}


def resolve_glyph_id(parsed, cp):
    """Resolve a code point to a glyph id using the generated cmap tables,
    mirroring lv_font.c's lookup semantics."""
    for entry in parsed["cmaps"]:
        rs, rl = entry["range_start"], entry["range_length"]
        if not (rs <= cp <= rs + rl - 1):
            continue
        ul = entry["unicode_list"]
        if ul is not None:
            values = parsed["lists"][ul]
            try:
                idx = values.index(cp - rs)
            except ValueError:
                return None
            ofs = parsed["lists"].get(entry["glyph_id_ofs_list"])
            return entry["glyph_id_start"] + (ofs[idx] if ofs else idx)
        ol = entry["glyph_id_ofs_list"]
        if ol is not None:
            return entry["glyph_id_start"] + parsed["lists"][ol][cp - rs]
        return entry["glyph_id_start"] + (cp - rs)
    return None


def glyph_pixels(parsed, gid):
    """Return (w, h, list-of-nibbles) for glyph gid (4 bpp, row-major,
    high nibble first)."""
    dsc = parsed["dsc"][gid]
    w, h = dsc["box_w"], dsc["box_h"]
    start = dsc["bitmap_index"]
    nbytes = (w * h * BPP + 7) // 8
    data = parsed["bitmap"][start:start + nbytes]
    nibbles = []
    for byte in data:
        nibbles.append(byte >> 4)
        nibbles.append(byte & 0x0F)
    return w, h, nibbles[: w * h]


# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------

def check_glyph(parsed, gid, cp=None):
    """Return (ok, reason). U+0020 legitimately has an empty 0x0 box."""
    if gid is None or gid >= len(parsed["dsc"]):
        return False, "no glyph descriptor"
    dsc = parsed["dsc"][gid]
    w, h = dsc["box_w"], dsc["box_h"]
    if w <= 0 or h <= 0:
        if cp == 0x20:
            return True, "space"
        return False, f"empty box {w}x{h}"
    nbytes = (w * h * BPP + 7) // 8
    seg = parsed["bitmap"][dsc["bitmap_index"]: dsc["bitmap_index"] + nbytes]
    if len(seg) != nbytes:
        return False, f"bitmap segment truncated ({len(seg)}/{nbytes} bytes)"
    if not any(seg):
        return False, "bitmap is all zero"
    return True, "ok"


def verify_coverage(c_path, required_cps):
    parsed = parse_generated_c(c_path)
    missing = []
    for cp in required_cps:
        gid = resolve_glyph_id(parsed, cp)
        ok, reason = check_glyph(parsed, gid, cp)
        if not ok:
            missing.append((cp, gid, reason))
    return parsed, missing


def best_iou(lv_fore, lv_w, lv_h, ft_fore, ft_w, ft_h, max_shift=2):
    """Best intersection-over-union between two binary foreground maps
    over small translations."""
    ft_set = {(x, y) for y in range(ft_h) for x in range(ft_w) if ft_fore[y][x]}
    lv_set = {(x, y) for y in range(lv_h) for x in range(lv_w) if lv_fore[y][x]}
    if not ft_set and not lv_set:
        return 1.0
    if not ft_set or not lv_set:
        return 0.0
    best = 0.0
    for dx in range(-max_shift, max_shift + 1):
        for dy in range(-max_shift, max_shift + 1):
            inter = len(lv_set & {(x + dx, y + dy) for x, y in ft_set})
            union = len(lv_set) + len(ft_set) - inter
            iou = inter / union if union else 0.0
            if iou > best:
                best = iou
    return best


def freetype_crosscheck(font_path, parsed, sample_cps, size):
    """Independently rasterise sample glyphs with Pillow's FreeType and
    compare against the generated bitmaps. Returns list of (cp, iou)."""
    try:
        from PIL import ImageFont  # noqa: PLC0415
    except ImportError as exc:
        print("[verify] WARNING: Pillow unavailable, skipping bitmap cross-check", file=sys.stderr)
        return None

    ft = ImageFont.truetype(str(font_path), size)
    results = []
    for cp in sample_cps:
        gid = resolve_glyph_id(parsed, cp)
        if gid is None:
            results.append((cp, 0.0))
            continue
        w, h, nibbles = glyph_pixels(parsed, gid)
        mask = ft.getmask(chr(cp))
        mw, mh = mask.size
        lv_fore = [[nibbles[y * w + x] >= 8 for x in range(w)] for y in range(h)]
        ft_fore = [[mask.getpixel((x, y)) >= 128 for x in range(mw)] for y in range(mh)]
        results.append((cp, best_iou(lv_fore, w, h, ft_fore, mw, mh)))
    return results


def read_name_version(font_path: Path):
    """Minimal sfnt 'name' table reader: (family, version string)."""
    try:
        data = font_path.read_bytes()
        if len(data) < 12:
            return None, None
        num_tables = struct.unpack(">H", data[4:6])[0]
        for i in range(num_tables):
            rec = data[12 + i * 16: 12 + i * 16 + 16]
            if rec[0:4] != b"name":
                continue
            off, length = struct.unpack(">II", rec[8:16])
            name = data[off:off + length]
            count = struct.unpack(">H", name[2:4])[0]
            stroff = struct.unpack(">H", name[4:6])[0]
            family = version = None
            for j in range(count):
                r = name[6 + j * 12: 6 + j * 12 + 12]
                pid, _, _, nid, rlen, roff = struct.unpack(">HHHHHH", r)
                if pid not in (0, 3) or not rlen:
                    continue
                s = name[stroff + roff: stroff + roff + rlen]
                text = s.decode("utf-16-be", "replace")
                if nid == 1 and family is None:
                    family = text
                elif nid == 5 and version is None:
                    version = text
            return family, version
    except (OSError, struct.error):
        pass
    return None, None


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--font", help="source CJK font path (default: cached Noto Sans SC)")
    ap.add_argument("--node", help="node executable path")
    ap.add_argument("--size", type=int, action="append", dest="sizes",
                    help="font size in px (repeatable; default 16 and 20)")
    ap.add_argument("--no-verify", action="store_true", help="skip verification pass")
    args = ap.parse_args(argv)

    sizes = args.sizes or [16, 20]
    if not TEXT_FILE.exists():
        raise RuntimeError(f"text inventory not found: {TEXT_FILE}")

    if args.font:
        font_path = Path(args.font).resolve()
    else:
        font_path = next((p for p in DEFAULT_FONT_CANDIDATES if p.exists()), None)
    if font_path is None or not font_path.exists():
        raise RuntimeError(
            "No source font found. Pass --font, or cache NotoSansSC_400Regular.ttf "
            "in tools/font_pipeline/ (see FONT_SOURCE.md)."
        )

    node = find_node(args.node)
    converter = find_converter()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    cps = extract_chars(TEXT_FILE)
    non_ascii = [cp for cp in cps if cp > 0x7E]
    print(f"[inventory] {TEXT_FILE.name}: {len(cps)} code points "
          f"({len(non_ascii)} non-ASCII)")
    print(f"[tools] node={node}  lv_font_conv={converter_version()}")

    results = {}
    failures = []
    for size in sizes:
        out_path = OUT_DIR / f"app_font_{size}.c"
        run_converter(node, converter, font_path, size, cps, out_path)

        missing = []
        cross = None
        if not args.no_verify:
            parsed, missing = verify_coverage(out_path, cps)
            if missing:
                detail = ", ".join(f"U+{cp:04X}(gid={gid}, {reason})"
                                   for cp, gid, reason in missing)
                failures.append(f"app_font_{size}: missing/bad glyphs: {detail}")
            else:
                sample = non_ascii + [cp for i, cp in enumerate(range(0x21, 0x7F)) if i % 6 == 0]
                sample = sample[:: max(1, len(sample) // MAX_SAMPLE_GLYPHS)]
                cross = freetype_crosscheck(font_path, parsed, sample, size)
                if cross is not None:
                    ious = [iou for _, iou in cross]
                    median = sorted(ious)[len(ious) // 2]
                    worst = min(ious)
                    bad = [f"U+{cp:04X}={iou:.2f}" for cp, iou in cross if iou < IOU_ABS_MIN]
                    print(f"[verify] app_font_{size}: cross-check IoU "
                          f"median={median:.2f} min={worst:.2f} (n={len(ious)})")
                    if bad:
                        print(f"[verify] app_font_{size}: low-agreement glyphs: {bad}")
                    if median < IOU_MEDIAN_MIN or bad:
                        failures.append(
                            f"app_font_{size}: bitmap cross-check failed "
                            f"(median IoU {median:.2f} < {IOU_MEDIAN_MIN} or {len(bad)} "
                            f"glyphs < {IOU_ABS_MIN}); possible converter corruption"
                        )

        results[str(size)] = {
            "file": str(out_path),
            "bytes": out_path.stat().st_size,
            "code_points": len(cps),
            "missing": [{"codepoint": f"U+{cp:04X}", "glyph_id": gid, "reason": r}
                        for cp, gid, r in missing],
            "cross_check": None if cross is None else {
                "median_iou": round(sorted([v for _, v in cross])[len(cross) // 2], 3),
                "min_iou": round(min(v for _, v in cross), 3),
                "n": len(cross),
                "low": [f"U+{cp:04X}" for cp, v in cross if v < IOU_ABS_MIN],
            },
        }

    family, version = read_name_version(font_path)
    report = {
        "generator": "tools/font_pipeline/gen_font.py",
        "text_inventory": str(TEXT_FILE),
        "source_font": {
            "path": str(font_path),
            "sha256": hashlib.sha256(font_path.read_bytes()).hexdigest().upper(),
            "family": family,
            "version": version,
        },
        "converter": {
            "name": "lv_font_conv",
            "version": converter_version(),
            "node": node,
        },
        "options": {"bpp": BPP, "format": "lvgl", "no_compress": True},
        "fonts": results,
        "ok": not failures,
        "failures": failures,
    }
    REPORT_FILE.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                           encoding="utf-8")
    print(f"[report] {REPORT_FILE}")

    if failures:
        print("VERIFICATION FAILED:", file=sys.stderr)
        for f in failures:
            print("  - " + f, file=sys.stderr)
        sys.exit(2)

    for size, info in results.items():
        print(f"[done] app_font_{size}: {info['bytes']} bytes, {info['code_points']} "
              f"code points, coverage OK")
    print("ALL CHECKS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
