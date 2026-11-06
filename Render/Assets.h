#ifndef RENDER_ASSETS_H
#define RENDER_ASSETS_H
#include <vector>
#include <string>

typedef unsigned int TextureID;

struct TextureData {
    const unsigned char* data;
    int size;
    int w, h, ch;
    std::string name;
};

extern const std::vector<TextureData> g_assetLibrary;
TextureID loadTextureFromMemory(const TextureData& asset);

// The scene is drawn from a single Canvas in a single draw call, so the
// library holds exactly one texture: the character atlas produced by
// Tools/gen_sprite_atlas.py (see Render/SpriteAtlas.h).
TextureID loadSpriteAtlas();
void freeTexture(TextureID id);
#endif
