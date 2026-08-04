#!/usr/bin/env python3
"""Regenerates boot/splash.raw from boot/assets/splash_source.png.

Run manually (`python3 boot/gen_splash.py`) whenever the source artwork
changes -- not part of the normal build, since it needs Pillow and the
freestanding bootloader can't decode PNG itself (see load_and_blit_splash's
comment in boot/boot.c for why). The output is a small header (magic +
width + height) followed by raw RGBA8 pixels, pre-resized here with a
quality resampler so boot.c only ever needs a straight memory copy --
no blocky nearest-neighbor scaling at boot time.
"""
import pathlib
import struct

from PIL import Image

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE / "assets" / "splash_source.png"
OUT = HERE / "splash.raw"

# Square, comfortably inside common GOP modes (e.g. QEMU/OVMF's default
# 1280x800) with margin to spare, while still downsampling the 1254px
# source only ~2x -- sharp, not pixelated.
TARGET_SIZE = 640


def main():
    im = Image.open(SRC).convert("RGBA")
    im = im.resize((TARGET_SIZE, TARGET_SIZE), Image.LANCZOS)
    pixels = im.tobytes()  # R,G,B,A per pixel, row-major

    with open(OUT, "wb") as f:
        f.write(struct.pack("<4sIII", b"ASPL", TARGET_SIZE, TARGET_SIZE, 0))
        f.write(pixels)

    print(f"wrote {OUT}: {TARGET_SIZE}x{TARGET_SIZE} RGBA "
          f"({len(pixels)} bytes pixel data, "
          f"{OUT.stat().st_size} bytes total)")


if __name__ == "__main__":
    main()
