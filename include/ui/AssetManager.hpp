#pragma once

#include <map>
#include <string>

#if NIMONSPOLY_ENABLE_RAYLIB
#include "ui/RaylibCompat.hpp"
#endif

class AssetManager {
public:
    static AssetManager& get();

    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

#if NIMONSPOLY_ENABLE_RAYLIB
    const Font& font(const std::string& key);
    const Texture2D* texture(const std::string& path);
#endif

    void loadAll();
    void unloadAll();

private:
    AssetManager() = default;
    ~AssetManager();

    bool loaded_{false};

#if NIMONSPOLY_ENABLE_RAYLIB
    std::map<std::string, Font>      fonts_;
    std::map<std::string, Texture2D> textures_;

    void loadFonts();
#endif
};
