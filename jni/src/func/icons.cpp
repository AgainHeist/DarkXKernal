#define STB_IMAGE_IMPLEMENTATION
#include "../other/stb_image.h"
#include "icons.hpp"
#include "../other/base64.hpp"
#include <iostream>

IconManager& IconManager::getInstance() {
    static IconManager instance;
    return instance;
}

IconManager::~IconManager() {
    for (auto const& [id, tex] : textureCache) {
        glDeleteTextures(1, &tex);
    }
    textureCache.clear();
}

GLuint IconManager::getIconTexture(int weaponId) {
    auto it = textureCache.find(weaponId);
    if (it != textureCache.end()) {
        return it->second;
    }

    // WeaponIcons system removed
    return 0;
}

GLuint IconManager::createTextureFromPNG(const unsigned char* data, int len) {
    int width, height, channels;
    unsigned char* image = stbi_load_from_memory(data, len, &width, &height, &channels, 4);
    if (!image) {
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);
    return textureID;
}

void IconManager::preloadAll() {
    // WeaponIcons system removed
}