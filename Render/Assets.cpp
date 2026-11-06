#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>

#include "Assets.h"
#include "SpriteAtlas.h"

#include <iostream>

// The only texture the scene uses. Everything else in the frame (lanes,
// sky, props, health bars) is drawn as flat coloured triangles that carry
// the UV (0,0) and therefore sample the atlas' white column.
const std::vector<TextureData> g_assetLibrary = {
    { kSpriteAtlasPng, static_cast<int>(kSpriteAtlasPngSize),
      kSpriteAtlasWidth, kSpriteAtlasHeight, 4, "character_atlas" }
};

TextureID loadTextureFromMemory(const TextureData& asset) {
    // Canvas UVs run bottom-up, so the image is flipped on the way in.
    stbi_set_flip_vertically_on_load(true);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(asset.data, asset.size,
                                                  &w, &h, &channels, 4);

    if (!pixels) {
        std::cerr << "couldnt load " << asset.name << ": "
                  << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // The atlas is packed edge to edge, so wrapping would pull colour in
    // from the opposite side, and the art must stay crisp: NEAREST only.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);
    return tex;
}

TextureID loadSpriteAtlas() {
    if (g_assetLibrary.empty()) return 0;
    return loadTextureFromMemory(g_assetLibrary[0]);
}

void freeTexture(TextureID id) {
    if (id != 0) glDeleteTextures(1, &id);
}
