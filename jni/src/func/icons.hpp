#pragma once
#include <string>
#include <map>
#include <GLES3/gl32.h>
#include <vector>
class IconManager {
public:
    static IconManager& getInstance();
    
    // Returns a GL texture ID for the given weapon ID.
    // If not loaded, it decodes the base64 PNG from WeaponIcons.h and creates a texture.
    GLuint getIconTexture(int weaponId);

    // Optional: Preload all icons (might be slow)
    void preloadAll();

private:
    IconManager() = default;
    ~IconManager();
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;

    std::map<int, GLuint> textureCache;
    GLuint createTextureFromPNG(const unsigned char* data, int len);
};
