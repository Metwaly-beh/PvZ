// GENERATED FILE - do not edit.
// Rebuild with:  python3 Tools/gen_sprite_atlas.py
#ifndef RENDER_SPRITE_ATLAS_H
#define RENDER_SPRITE_ATLAS_H

// One image holding every character frame plus a solid white
// column on the left edge. Untextured Canvas vertices carry the
// UV (0,0) and therefore sample that white column, which is why
// the environment keeps working unchanged.
struct SpriteSheet {
    const char* name;
    int x, y;       // pixel origin of the strip inside the atlas
    int fw, fh;     // frame size in pixels
    int cols, rows; // frames are addressed row-major
    int flipX;      // 1 = mirror horizontally when drawing
};

extern const unsigned char  kSpriteAtlasPng[];
extern const unsigned int   kSpriteAtlasPngSize;
extern const int            kSpriteAtlasWidth;
extern const int            kSpriteAtlasHeight;
extern const SpriteSheet    kSpriteSheets[];
extern const int             kSpriteSheetCount;

// nullptr when the name is unknown.
const SpriteSheet* findSpriteSheet(const char* name);

#endif // RENDER_SPRITE_ATLAS_H
