#!/usr/bin/env python3
"""
img2anim.py — Convert ANI files to a multi-animation Meshtastic C header.

Each ANI file becomes one named animation with its actual frame count.
All animations share the same pixel dimensions (64×64 by default).

Inputs (--input):
  - A directory path   → all *.ani files inside (sorted alphabetically)
  - A comma-separated list of ANI / image files

Output: anim_frames.h containing:
  - One flat uint8_t array per animation: ANIM_DATA_BUSY[], etc.
  - An AnimDesc descriptor struct
  - ANIM_LIST[ANIM_COUNT] table with {name, data, frameCount}
  - ANIM_FRAME_WIDTH / ANIM_FRAME_HEIGHT / ANIM_BYTES_PER_FRAME constants

Usage examples:
    python3 img2anim.py --input /path/to/cursors/
    python3 img2anim.py --input pointer.ani,busy.ani --width 64 --height 64
    python3 img2anim.py --input /path/to/cursors/ --preview --output anim_frames.h
"""

import argparse
import io
import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow not installed. Run: pip install Pillow", file=sys.stderr)
    sys.exit(1)


# ---------------------------------------------------------------------------
# ANI parser – extract ALL frames
# ---------------------------------------------------------------------------


def _iter_chunks(buf: bytes, offset: int):
    """Yield (chunk_id, chunk_data) pairs from a RIFF chunk stream."""
    while offset + 8 <= len(buf):
        cid = buf[offset : offset + 4]
        csz = struct.unpack_from("<I", buf, offset + 4)[0]
        cdata = buf[offset + 8 : offset + 8 + csz]
        yield cid, cdata
        offset += 8 + csz + (csz & 1)


def extract_ani_frames(path: Path) -> list:
    """Return a list of PIL Images – one per frame in the .ani file."""
    data = path.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"ACON":
        raise ValueError(f"Not a valid .ani file: {path}")
    frames = []
    for cid, cdata in _iter_chunks(data, 12):
        if cid == b"LIST" and cdata[:4] == b"fram":
            for sub_id, sub_data in _iter_chunks(cdata, 4):
                if sub_id == b"icon":
                    frames.append(Image.open(io.BytesIO(sub_data)))
    if not frames:
        raise ValueError(f"No 'icon' frames found in {path}")
    return frames


def load_frames(path: Path) -> list:
    """Load all frames from an ANI or a single static image."""
    if path.suffix.lower() == ".ani":
        return extract_ani_frames(path)
    return [Image.open(path)]


# ---------------------------------------------------------------------------
# Image → XBM bytes
# ---------------------------------------------------------------------------


def image_to_xbm_bytes(
    img: Image.Image, width: int, height: int, threshold: int, invert: bool
) -> list:
    """Resize + threshold → XBM bytes (LSB-first, row-major)."""
    if img.mode in ("RGBA", "LA"):
        bg = Image.new("RGB", img.size, (255, 255, 255))
        bg.paste(img.convert("RGB"), mask=img.split()[-1])
        img = bg
    elif img.mode == "PA":
        img = img.convert("RGBA")
        bg = Image.new("RGB", img.size, (255, 255, 255))
        bg.paste(img.convert("RGB"), mask=img.split()[-1])
        img = bg
    elif img.mode not in ("RGB", "L"):
        img = img.convert("RGB")

    img = img.resize((width, height), Image.LANCZOS).convert("L")
    pix = img.load()
    bpr = (width + 7) // 8
    data = []
    for y in range(height):
        for bx in range(bpr):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < width:
                    dark = pix[x, y] <= threshold
                    if invert:
                        dark = not dark
                    if dark:
                        byte |= 1 << bit
            data.append(byte)
    return data


# ---------------------------------------------------------------------------
# ASCII preview
# ---------------------------------------------------------------------------


def ascii_preview(data: list, width: int, height: int, label: str):
    bpr = (width + 7) // 8
    print(f"  ── {label} ──", file=sys.stderr)
    for y in range(height):
        row = "".join(
            "█" if (data[y * bpr + x // 8] >> (x % 8)) & 1 else " "
            for x in range(width)
        )
        print(f"  |{row}|", file=sys.stderr)


# ---------------------------------------------------------------------------
# C symbol name helpers
# ---------------------------------------------------------------------------


def to_c_name(stem: str) -> str:
    """'dgn1' → 'DGN1', 'busy' → 'BUSY' (safe C identifier, uppercased)."""
    safe = "".join(c if c.isalnum() else "_" for c in stem).upper()
    if safe and safe[0].isdigit():
        safe = "_" + safe
    return safe


# ---------------------------------------------------------------------------
# C header generation
# ---------------------------------------------------------------------------


def _flat_array_lines(frames_xbm: list, width: int, array_name: str) -> list:
    """Emit 'static const uint8_t NAME[] = { ... };' lines."""
    bpr = (width + 7) // 8
    lines = [f"static const uint8_t {array_name}[] = {{"]
    total_bytes = len(frames_xbm) * bpr * (len(frames_xbm[0]) // bpr)
    byte_idx = 0
    for fi, frame in enumerate(frames_xbm):
        lines.append(f"    /* frame {fi} */")
        for ri in range(len(frame) // bpr):
            row = frame[ri * bpr : ri * bpr + bpr]
            hex_str = ", ".join(f"0x{b:02X}" for b in row)
            byte_idx += bpr
            comma = "," if byte_idx < total_bytes else ""
            lines.append(f"    /* r{ri:3d} */ {hex_str}{comma}")
    lines.append("};")
    return lines


def generate_header(
    animations: list,
    width: int,
    height: int,
    src_desc: str,
    threshold: int,
    invert: bool,
    show_preview: bool,
) -> str:
    """Generate a C header string containing all animation frame data.

    animations: list of (stem_name, [frame_xbm_bytes, ...])
      stem_name – filename without extension (e.g. 'busy')
      frame_xbm_bytes – list of flat XBM byte lists, one per frame
    """
    bpr = (width + 7) // 8
    bytes_per_frame = bpr * height

    out = []
    out.append("// Auto-generated by tools/img2anim.py — re-run the script to update.")
    out.append(f"// Source: {src_desc}")
    out.append("//")
    out.append("// XBM format: LSB-first, row-major.")
    out.append(f"//   width         = {width} px  →  {bpr} bytes/row")
    out.append(f"//   height        = {height} px")
    out.append(f"//   bytes/frame   = {bytes_per_frame}")
    out.append(f"//   animations    = {len(animations)}")
    out.append("")
    out.append("#pragma once")
    out.append("#include <stdint.h>")
    out.append("")
    out.append(f"#define ANIM_FRAME_WIDTH    {width}")
    out.append(f"#define ANIM_FRAME_HEIGHT   {height}")
    out.append(f"#define ANIM_BYTES_PER_FRAME {bytes_per_frame}")
    out.append(f"#define ANIM_COUNT          {len(animations)}")
    out.append("")
    out.append("// clang-format off")
    out.append("")

    # Per-animation flat arrays
    for stem, frames in animations:
        cname = f"ANIM_DATA_{to_c_name(stem)}"
        out.append(f"// {stem}  ({len(frames)} frame(s))")
        out.extend(_flat_array_lines(frames, width, cname))
        out.append("")

    # Descriptor struct + table
    out.append("struct AnimDesc {")
    out.append("    const char    *name;")
    out.append("    const uint8_t *data;")
    out.append("    uint8_t        frameCount;")
    out.append("};")
    out.append("")
    out.append("static const AnimDesc ANIM_LIST[ANIM_COUNT] = {")
    for i, (stem, frames) in enumerate(animations):
        cname = f"ANIM_DATA_{to_c_name(stem)}"
        comma = "," if i < len(animations) - 1 else ""
        out.append(f'    {{ "{stem}", {cname}, {len(frames)} }}{comma}')
    out.append("};")
    out.append("")
    out.append("// clang-format on")
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main():
    ap = argparse.ArgumentParser(
        description="Convert ANI files to a multi-animation Meshtastic C header."
    )
    ap.add_argument(
        "--input",
        "-i",
        required=True,
        help="Directory of *.ani files, or comma-separated file list",
    )
    ap.add_argument(
        "--width",
        "-W",
        type=int,
        default=64,
        help="Output width  in pixels (default: 64)",
    )
    ap.add_argument(
        "--height",
        "-H",
        type=int,
        default=64,
        help="Output height in pixels (default: 64)",
    )
    ap.add_argument(
        "--threshold",
        "-t",
        type=int,
        default=128,
        help="Greyscale threshold 0-255 (default: 128)",
    )
    ap.add_argument(
        "--invert", "-v", action="store_true", help="Invert foreground/background"
    )
    ap.add_argument(
        "--preview",
        "-p",
        action="store_true",
        help="Print ASCII preview of first frame of each animation",
    )
    ap.add_argument(
        "--output", "-o", help="Output .h file (default: anim_frames.h in CWD)"
    )
    args = ap.parse_args()

    # ── Collect input paths ───────────────────────────────────────────────
    inp = Path(args.input)
    if inp.is_dir():
        paths = sorted(inp.glob("*.ani"))
        if not paths:
            print(f"ERROR: no .ani files found in {inp}", file=sys.stderr)
            sys.exit(1)
        src_desc = str(inp)
    else:
        paths = [Path(p.strip()) for p in args.input.split(",")]
        for p in paths:
            if not p.exists():
                print(f"ERROR: file not found: {p}", file=sys.stderr)
                sys.exit(1)
        src_desc = args.input

    # ── Load + convert each animation ─────────────────────────────────────
    animations = []  # list of (stem, [xbm_bytes, ...])
    total_frames = 0
    for path in paths:
        src_frames = load_frames(path)
        xbm_frames = [
            image_to_xbm_bytes(f, args.width, args.height, args.threshold, args.invert)
            for f in src_frames
        ]
        animations.append((path.stem, xbm_frames))
        total_frames += len(xbm_frames)
        print(f"  {path.name:<22s}  {len(xbm_frames):2d} frame(s)", file=sys.stderr)
        if args.preview:
            ascii_preview(
                xbm_frames[0], args.width, args.height, f"{path.stem} frame 0"
            )

    total_bytes = total_frames * ((args.width + 7) // 8) * args.height
    print(
        f"\n  {len(animations)} animations, {total_frames} frames total, "
        f"{total_bytes} bytes in flash",
        file=sys.stderr,
    )

    # ── Write header ──────────────────────────────────────────────────────
    header = generate_header(
        animations,
        args.width,
        args.height,
        src_desc,
        args.threshold,
        args.invert,
        args.preview,
    )

    out_path = Path(args.output) if args.output else Path("anim_frames.h")
    out_path.write_text(header, encoding="utf-8")
    print(f"Written: {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
