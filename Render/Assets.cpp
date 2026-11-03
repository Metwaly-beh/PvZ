#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Assets.h"
#include "Renderer.h"
#include <iostream>
#include "test_asset.h" 

const std::vector<TextureData> g_assetLibrary = {
    { test_png_data, 1024, 256, 256, 4, "test_sprite" } 
};

TextureID loadTextureFromMemory(const TextureData& asset) {
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load_from_memory(asset.data, asset.size, nullptr, nullptr, nullptr, 4);
    
    if(!pixels) {
        std::cerr << "couldnt load " << asset.name << std::endl;
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, asset.w, asset.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    stbi_image_free(pixels);
    return tex;
}
