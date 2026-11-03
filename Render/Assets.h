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
#endif
