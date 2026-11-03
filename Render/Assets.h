#ifndef RENDER_ASSETS_H
#define RENDER_ASSETS_H

#include <vector>
#include <string>

using TextureID = unsigned int;

struct TextureData {
    const unsigned char* data;
    int width;
    int height;
    int channels;
    std::string name;
};

extern const std::vector<TextureData> g_assetLibrary;

TextureID loadTextureFromMemory(const TextureData& asset);

#endif // RENDER_ASSETS_H
