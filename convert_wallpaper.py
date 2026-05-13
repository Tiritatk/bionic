#!/usr/bin/env python3
from PIL import Image
import sys
from pathlib import Path

if len(sys.argv) < 3:
    print("Usage: python3 convert_wallpaper.py input.png output_wallpaper.h [width] [height]")
    print("Example: python3 convert_wallpaper.py my_wallpaper.png include/wallpaper.h 320 180")
    sys.exit(1)

inp = Path(sys.argv[1])
outp = Path(sys.argv[2])
w = int(sys.argv[3]) if len(sys.argv) > 3 else 320
h = int(sys.argv[4]) if len(sys.argv) > 4 else 180

img = Image.open(inp).convert("RGB").resize((w, h), Image.LANCZOS)
pixels = list(img.getdata())

lines = []
lines.append("#ifndef BIONIC_WALLPAPER_H")
lines.append("#define BIONIC_WALLPAPER_H")
lines.append("")
lines.append("#include <stdint.h>")
lines.append("")
lines.append(f"#define BIONIC_WALLPAPER_WIDTH {w}")
lines.append(f"#define BIONIC_WALLPAPER_HEIGHT {h}")
lines.append("")
lines.append("static const uint32_t bionic_wallpaper[BIONIC_WALLPAPER_WIDTH * BIONIC_WALLPAPER_HEIGHT] = {")

values = []
for r, g, b in pixels:
    values.append(0xFF000000 | (r << 16) | (g << 8) | b)

for i in range(0, len(values), 8):
    lines.append("    " + ", ".join(f"0x{v:08X}" for v in values[i:i+8]) + ",")

lines.append("};")
lines.append("")
lines.append("#endif")

outp.write_text("\n".join(lines))
print(f"Wrote {outp} ({w}x{h}, {len(values)} pixels)")
