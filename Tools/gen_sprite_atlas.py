#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# gen_sprite_atlas.py
#
# Composes every character sprite the game needs into ONE atlas texture and
# emits Render/SpriteAtlas.h + Render/SpriteAtlasData.cpp.
#
# Why an atlas: the scene renderer uploads a Canvas as a single vertex buffer
# and draws it with one glDrawArrays call, so there is no point in the frame
# where a texture could be swapped. Packing every sheet into one image means
# the whole scene (environment + all six character types) is textured by a
# single bind.
#
# Layout rules:
#   * the leftmost WHITE_COLS pixel columns are solid white, so the UV (0,0)
#     that every untextured Canvas vertex carries samples white and the
#     environment keeps drawing exactly as before;
#   * each character type occupies one horizontal strip, frames in the order
#     [idle][walk][attack][death], which is what CharArt's SpriteAnim ranges
#     index into;
#   * rows are separated by GAP pixels so NEAREST sampling at a frame edge can
#     never bleed into the neighbouring strip.
#
# Run from anywhere:  python3 Tools/gen_sprite_atlas.py
# ---------------------------------------------------------------------------

import os
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_H = os.path.join(ROOT, "Render", "SpriteAtlas.h")
OUT_CPP = os.path.join(ROOT, "Render", "SpriteAtlasData.cpp")

WHITE_COLS = 4   # solid white column on the left edge -> untextured UVs
GAP = 8          # vertical padding between strips

A = "Assets"
KNIGHT = A + "/extracted/knights/little/LittleKnight/sheets/animations/"
EGYPT = A + "/extracted/Egypt_Characters_-_Sprite_Sheets/"
BUGS = A + "/extracted/Animated_insect_enemy_assets/"

ADV_SHEET = A + "/Adventurer_Sprite_Sheet_v1.5.png"        # 416x480, 13x15
FEM_SHEET = A + "/Adventurer_Female_Sprite_Sheet.png"      # 288x224, 9x7
MUMMY_SHEET = EGYPT + "Mummy/Mummy.png"                    # 2496x96, 26 frames


def load(path):
    im = Image.open(os.path.join(ROOT, path))
    return im.convert("RGBA")


def grid(path, fw, fh):
    """Return the sheet as rows of frames: grid[r][c]."""
    im = load(path)
    cols, rows = im.width // fw, im.height // fh
    return [[im.crop((c * fw, r * fh, (c + 1) * fw, (r + 1) * fh))
             for c in range(cols)] for r in range(rows)]


def strip(path, fw):
    """A single-row sheet of frames."""
    im = load(path)
    return [im.crop((i * fw, 0, (i + 1) * fw, im.height))
            for i in range(im.width // fw)]


def build_roles():
    """[(name, frame_size, flipX, [(anim, frames), ...]), ...] in draw order."""
    adv = grid(ADV_SHEET, 32, 32)
    fem = grid(FEM_SHEET, 32, 32)
    mum = grid(MUMMY_SHEET, 96, 96)[0]

    # Ponk's insect sheets are top-down: row 1 is the side view. Both the
    # walk and the idle strip use it so the beetle always reads as moving
    # along the lane, and it faces left, which is the way enemies travel.
    bug_walk = grid(BUGS + "BeetleMove.png", 32, 32)[1]
    bug_attack = grid(BUGS + "BeetleAttack.png", 32, 32)[1]

    return [
        # --- players: they advance to the right, so they are drawn as-is ---
        ("p_melee", 32, False, [
            ("idle",   adv[0][0:13]),
            ("walk",   adv[1][0:8]),
            ("attack", adv[2][0:10]),
            ("death",  adv[7][0:7] + adv[8][0:4]),
        ]),
        ("p_ranged", 32, False, [
            ("idle",   fem[0][0:6]),
            ("walk",   fem[1][0:8]),
            ("attack", fem[2][0:9]),
            ("death",  fem[4][0:7]),
        ]),
        ("p_tank", 32, False, [
            ("idle",   strip(KNIGHT + "knight_idle_strip6.png", 32)),
            ("walk",   strip(KNIGHT + "knight_walk_strip8.png", 32)),
            ("attack", strip(KNIGHT + "knight_attack_1_strip5.png", 32)),
            ("death",  strip(KNIGHT + "knight_death_strip8.png", 32)),
        ]),

        # --- enemies: they advance to the left -----------------------------
        # DvoraGames mummy, straight from its Aseprite JSON frame order:
        # Idle 0-3, Run 4-8, Attack 9-14, Hit 15-19, Dead 20-25.
        ("e_tank", 96, False, [
            ("idle",   mum[0:4]),
            ("walk",   mum[4:9]),
            ("attack", mum[9:15]),
            ("death",  mum[20:26]),
        ]),
        # The skeleton pack faces right, so it is mirrored for the enemy side.
        ("e_ranged", 32, True, [
            ("idle",   strip(KNIGHT + "skeleton_idle_strip4.png", 32)),
            ("walk",   strip(KNIGHT + "skeleton_walk_strip6.png", 32)),
            ("attack", strip(KNIGHT + "skeleton_attack_strip6.png", 32)),
            ("death",  strip(KNIGHT + "skeleton_death_strip7.png", 32)),
        ]),
        # The beetle pack ships no death animation; the last attack frame is
        # reused as the resting corpse frame (dead characters are removed on
        # the same tick anyway, so it is only ever seen for one frame).
        ("e_swarm", 32, False, [
            ("idle",   bug_walk),
            ("walk",   bug_walk),
            ("attack", bug_attack),
            ("death",  [bug_attack[5]]),
        ]),
    ]


def compose(roles):
    strips = []
    for name, fw, flip, anims in roles:
        frames = [f for _, fs in anims for f in fs]
        assert frames, name
        assert all(f.size == (fw, fw) for f in frames), name
        w = fw * len(frames)
        sheet = Image.new("RGBA", (w, fw), (0, 0, 0, 0))
        for i, f in enumerate(frames):
            sheet.alpha_composite(f, (i * fw, 0))
        strips.append((name, fw, flip, anims, sheet))

    atlas_w = WHITE_COLS + max(s[4].width for s in strips)
    atlas_h = sum(s[4].height for s in strips) + GAP * (len(strips) - 1)

    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    white = Image.new("RGBA", (WHITE_COLS, atlas_h), (255, 255, 255, 255))
    atlas.paste(white, (0, 0))

    y = 0
    meta = []
    for name, fw, flip, anims, sheet in strips:
        atlas.alpha_composite(sheet, (WHITE_COLS, y))
        total = sum(len(fs) for _, fs in anims)
        meta.append({
            "name": name, "x": WHITE_COLS, "y": y,
            "fw": fw, "fh": fw, "cols": total, "rows": 1,
            "flip": flip, "anims": anims,
        })
        y += sheet.height + GAP

    return atlas, meta


def emit(atlas, meta):
    atlas_path = os.path.join(ROOT, "Assets", "generated_character_atlas.png")
    atlas.save(atlas_path)
    W, H = atlas.size

    png = open(atlas_path, "rb").read()

    h = []
    h.append("// GENERATED FILE - do not edit.")
    h.append("// Rebuild with:  python3 Tools/gen_sprite_atlas.py")
    h.append("#ifndef RENDER_SPRITE_ATLAS_H")
    h.append("#define RENDER_SPRITE_ATLAS_H")
    h.append("")
    h.append("// One image holding every character frame plus a solid white")
    h.append("// column on the left edge. Untextured Canvas vertices carry the")
    h.append("// UV (0,0) and therefore sample that white column, which is why")
    h.append("// the environment keeps working unchanged.")
    h.append("struct SpriteSheet {")
    h.append("    const char* name;")
    h.append("    int x, y;       // pixel origin of the strip inside the atlas")
    h.append("    int fw, fh;     // frame size in pixels")
    h.append("    int cols, rows; // frames are addressed row-major")
    h.append("    int flipX;      // 1 = mirror horizontally when drawing")
    h.append("};")
    h.append("")
    h.append("extern const unsigned char  kSpriteAtlasPng[];")
    h.append("extern const unsigned int   kSpriteAtlasPngSize;")
    h.append("extern const int            kSpriteAtlasWidth;")
    h.append("extern const int            kSpriteAtlasHeight;")
    h.append("extern const SpriteSheet    kSpriteSheets[];")
    h.append("extern const int             kSpriteSheetCount;")
    h.append("")
    h.append("// nullptr when the name is unknown.")
    h.append("const SpriteSheet* findSpriteSheet(const char* name);")
    h.append("")
    h.append("#endif // RENDER_SPRITE_ATLAS_H")
    with open(OUT_H, "w") as f:
        f.write("\n".join(h) + "\n")

    c = []
    c.append("// GENERATED FILE - do not edit.")
    c.append("// Rebuild with:  python3 Tools/gen_sprite_atlas.py")
    c.append('#include "SpriteAtlas.h"')
    c.append("")
    c.append("const int kSpriteAtlasWidth  = %d;" % W)
    c.append("const int kSpriteAtlasHeight = %d;" % H)
    c.append("")
    c.append("const unsigned char kSpriteAtlasPng[] = {")
    for i in range(0, len(png), 20):
        chunk = png[i:i + 20]
        c.append("    " + ", ".join("0x%02x" % b for b in chunk) + ",")
    c.append("};")
    c.append("")
    c.append("const unsigned int kSpriteAtlasPngSize =")
    c.append("    (unsigned int)sizeof(kSpriteAtlasPng);")
    c.append("")
    c.append("const SpriteSheet kSpriteSheets[] = {")
    for m in meta:
        c.append('    { "%s", %d, %d, %d, %d, %d, %d, %d },'
                 % (m["name"], m["x"], m["y"], m["fw"], m["fh"],
                    m["cols"], m["rows"], 1 if m["flip"] else 0))
    c.append("};")
    c.append("")
    c.append("const int kSpriteSheetCount =")
    c.append("    (int)(sizeof(kSpriteSheets) / sizeof(kSpriteSheets[0]));")
    c.append("")
    c.append("const SpriteSheet* findSpriteSheet(const char* name)")
    c.append("{")
    c.append("    if (!name) return 0;")
    c.append("    for (int i = 0; i < kSpriteSheetCount; ++i) {")
    c.append("        const char* a = kSpriteSheets[i].name;")
    c.append("        const char* b = name;")
    c.append("        while (*a && *b && *a == *b) { ++a; ++b; }")
    c.append("        if (*a == 0 && *b == 0) return &kSpriteSheets[i];")
    c.append("    }")
    c.append("    return 0;")
    c.append("}")
    with open(OUT_CPP, "w") as f:
        f.write("\n".join(c) + "\n")

    print("atlas  %dx%d  (%d bytes png)" % (W, H, len(png)))
    print("wrote  Render/SpriteAtlas.h")
    print("wrote  Render/SpriteAtlasData.cpp")
    for m in meta:
        total = sum(len(fs) for _, fs in m["anims"])
        ranges = ", ".join("%s=%d..%d" % (n, off, off + len(fs) - 1)
                           for (n, fs), off in zip(
                               m["anims"],
                               _offsets(m["anims"])))
        print("  %-9s %d frames  %s%s" % (m["name"], total, ranges,
                                          "  (flipped)" if m["flip"] else ""))


def _offsets(anims):
    out, n = [], 0
    for _, fs in anims:
        out.append(n)
        n += len(fs)
    return out


def main():
    roles = build_roles()
    atlas, meta = compose(roles)
    emit(atlas, meta)


if __name__ == "__main__":
    sys.exit(main())
