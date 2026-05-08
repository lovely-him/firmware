#!/usr/bin/env python3
r"""
img2xbm.py — Convert an image to a Meshtastic OEM splash screen header.

Supported input formats: PNG, JPG, BMP, ICO, CUR, ANI (Windows animated cursor).

Usage examples:
    python3 img2xbm.py --input pointer.ani  --width 64 --height 64
    python3 img2xbm.py --input logo.png     --width 80 --height 60 --invert
    python3 img2xbm.py --input icon.png     --width 64 --height 64 \\
                       --text "MY DEVICE"  --output ../oem_splash.h
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
# ANI (Windows Animated Cursor) parser
# ---------------------------------------------------------------------------


def _iter_chunks(buf: bytes, offset: int):
    """Yield (chunk_id, chunk_data) pairs from a RIFF chunk stream."""
    while offset + 8 <= len(buf):
        cid = buf[offset : offset + 4]
        csz = struct.unpack_from("<I", buf, offset + 4)[0]
        cdata = buf[offset + 8 : offset + 8 + csz]
        yield cid, cdata
        offset += 8 + csz + (csz & 1)  # RIFF chunks are word-aligned


def extract_ani_first_frame(path: Path) -> Image.Image:
    """Extract the first icon frame from a Windows .ani RIFF file."""
    data = path.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"ACON":
        raise ValueError(f"Not a valid .ani file: {path}")

    for cid, cdata in _iter_chunks(data, 12):
        if cid == b"LIST" and cdata[:4] == b"fram":
            for sub_id, sub_data in _iter_chunks(cdata, 4):
                if sub_id == b"icon":
                    return Image.open(io.BytesIO(sub_data))

    raise ValueError(f"No 'icon' frame found inside {path}")


# ---------------------------------------------------------------------------
# Image → XBM byte list
# ---------------------------------------------------------------------------


def load_image(path: Path) -> Image.Image:
    """Load any supported image format."""
    if path.suffix.lower() == ".ani":
        return extract_ani_first_frame(path)
    return Image.open(path)


def image_to_xbm_bytes(
    img: Image.Image, width: int, height: int, threshold: int, invert: bool
) -> list[int]:
    """
    Resize/threshold the image and return XBM-format bytes (LSB-first, row-major).

    XBM convention: bit=1 is the foreground (drawn) colour.
    In Meshtastic E-ink, the foreground colour is WHITE (display-level inversion),
    so a dark logo on a light source image may need --invert to appear correctly.
    """
    # Flatten transparency onto a white background
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

    img = img.resize((width, height), Image.LANCZOS)
    img = img.convert("L")  # grayscale

    pix = img.load()
    bytes_per_row = (width + 7) // 8
    data = []

    for y in range(height):
        for bx in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < width:
                    is_dark = pix[x, y] <= threshold
                    if invert:
                        is_dark = not is_dark
                    if is_dark:
                        byte |= 1 << bit  # XBM is LSB-first
            data.append(byte)

    return data


# ---------------------------------------------------------------------------
# C header generation
# ---------------------------------------------------------------------------


def _format_data_block(data: list[int], bytes_per_row: int) -> str:
    """
    Format XBM bytes as a C initializer body, one pixel-row per source line.
    Returns lines WITHOUT the enclosing { } or trailing backslashes.
    """
    lines = []
    for row_start in range(0, len(data), bytes_per_row):
        row = data[row_start : row_start + bytes_per_row]
        row_idx = row_start // bytes_per_row
        hex_vals = ",".join(f"0x{b:02X}" for b in row)
        lines.append(f"    /* row {row_idx:3d} */ {hex_vals}")
    return lines


def generate_header(
    data: list[int], width: int, height: int, oem_text: str, font_size: int
) -> str:
    bytes_per_row = (width + 7) // 8
    total = len(data)
    body_lines = _format_data_block(data, bytes_per_row)

    # Build the #define with continuation backslashes
    define_lines = ["// clang-format off"]
    define_lines.append("#define USERPREFS_OEM_IMAGE_DATA { \\")
    for i, line in enumerate(body_lines):
        comma = "," if i < len(body_lines) - 1 else " "
        define_lines.append(line + comma + " \\")
    define_lines.append("}")
    define_lines.append("// clang-format on")

    header = f"""\
// Auto-generated by tools/img2xbm.py — re-run the script to update.
//
// XBM format: LSB-first, row-major.
//   width  = {width} px  →  {bytes_per_row} bytes/row
//   height = {height} px  →  {height} rows
//   total  = {total} bytes
//
// If stripes / the logo appear with inverted colours on the E-ink panel,
// re-run with the --invert flag and regenerate.

#pragma once

#define USERPREFS_OEM_TEXT         "{oem_text}"
#define USERPREFS_OEM_FONT_SIZE    {font_size}
#define USERPREFS_OEM_IMAGE_WIDTH  {width}
#define USERPREFS_OEM_IMAGE_HEIGHT {height}

"""
    return header + "\n".join(define_lines) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="Convert image → Meshtastic OEM splash screen header (XBM)."
    )

    parser.add_argument(
        "--input", "-i", required=True, help="Input image: PNG/JPG/BMP/ICO/ANI"
    )
    parser.add_argument(
        "--width",
        "-W",
        type=int,
        default=64,
        help="Output width  in pixels (default: 64)",
    )
    parser.add_argument(
        "--height",
        "-H",
        type=int,
        default=64,
        help="Output height in pixels (default: 64)",
    )
    parser.add_argument(
        "--threshold",
        "-t",
        type=int,
        default=128,
        help="Greyscale threshold 0-255; pixels ≤ value → foreground (default: 128)",
    )
    parser.add_argument(
        "--invert",
        "-v",
        action="store_true",
        help="Invert foreground/background (use if E-ink shows logo inverted)",
    )
    parser.add_argument(
        "--text",
        "-T",
        default="MY DEVICE",
        help='OEM text shown below the logo (default: "MY DEVICE")',
    )
    parser.add_argument(
        "--font-size",
        "-f",
        type=int,
        default=1,
        choices=[0, 1, 2],
        help="Font size: 0=small 1=medium 2=large (default: 1)",
    )
    parser.add_argument(
        "--output", "-o", help="Output .h file path (default: print to stdout)"
    )
    parser.add_argument(
        "--preview",
        "-p",
        action="store_true",
        help="Print an ASCII preview of the 1-bit image to stderr",
    )

    args = parser.parse_args()

    src = Path(args.input)
    if not src.exists():
        print(f"ERROR: input file not found: {src}", file=sys.stderr)
        sys.exit(1)

    print(f"Loading {src} …", file=sys.stderr)
    img = load_image(src)
    print(f"  original size: {img.size}, mode: {img.mode}", file=sys.stderr)

    data = image_to_xbm_bytes(img, args.width, args.height, args.threshold, args.invert)

    if args.preview:
        # ASCII preview — shows what will appear on screen
        bpr = (args.width + 7) // 8
        print("\n--- ASCII preview (# = foreground / set bit) ---", file=sys.stderr)
        for row in range(args.height):
            line = ""
            for bx in range(bpr):
                byte = data[row * bpr + bx]
                for bit in range(8):
                    x = bx * 8 + bit
                    if x < args.width:
                        line += "#" if (byte >> bit) & 1 else "."
            print(line, file=sys.stderr)
        print("---", file=sys.stderr)

    header = generate_header(data, args.width, args.height, args.text, args.font_size)

    if args.output:
        out = Path(args.output)
        out.write_text(header, encoding="utf-8")
        print(f"Written to {out}", file=sys.stderr)
    else:
        print(header)


if __name__ == "__main__":
    main()
