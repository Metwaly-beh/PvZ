#!/usr/bin/env python3
"""Verify the generated atlas: white column, strip placement, frame fidelity."""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
ROOT = "/Users/macbook/Desktop/PvZ"
sys.path.insert(0, os.path.join(ROOT, "Tools"))

from PIL import Image, ImageChops
import gen_sprite_atlas as g

atlas = Image.open(os.path.join(ROOT, "Assets", "generated_character_atlas.png")).convert("RGBA")
W, H = atlas.size
px = atlas.load()
errors = []

# 1. the leftmost white columns must be solid white over the whole height
for x in range(g.WHITE_COLS):
    for y in range(H):
        if px[x, y] != (255, 255, 255, 255):
            errors.append("white column broken at (%d,%d) = %r" % (x, y, px[x, y]))
            break

roles = g.build_roles()
strips = []
for name, fw, flip, anims in roles:
    frames = [f for _, fs in anims for f in fs]
    strips.append((name, fw, flip, anims, frames))

# 2. every strip must sit at (WHITE_COLS, y) and every frame must match the source
y = 0
for name, fw, flip, anims, frames in strips:
    sheet_w = fw * len(frames)
    for i, expected in enumerate(frames):
        got = atlas.crop((g.WHITE_COLS + i * fw, y, g.WHITE_COLS + (i + 1) * fw, y + fw))
        if got.size != expected.size:
            errors.append("%s frame %d size %r != %r" % (name, i, got.size, expected.size))
            continue
        diff = ImageChops.difference(got, expected).getbbox()
        if diff is not None:
            errors.append("%s frame %d differs from source (bbox %r)" % (name, i, diff))
    y += fw + g.GAP

# 3. no strip may run past the atlas
if y - g.GAP > H:
    errors.append("strips overflow the atlas: %d > %d" % (y - g.GAP, H))

print("atlas %dx%d" % (W, H))
print("strips checked: %d, frames checked: %d"
      % (len(strips), sum(len(s[4]) for s in strips)))
if errors:
    print("FAILED (%d):" % len(errors))
    for e in errors[:20]:
        print("  " + e)
    sys.exit(1)
print("OK - white column intact, all frames match their sources")
