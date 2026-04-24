#include "ui/AssetManager.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

AssetManager& AssetManager::get() {
    static AssetManager instance;
    return instance;
}

AssetManager::~AssetManager() {
    unloadAll();
}

void AssetManager::loadAll() {
    if (loaded_) {
        return;
    }
    loaded_ = true;

#if NIMONSPOLY_ENABLE_RAYLIB
    loadFonts();
#endif
}

void AssetManager::unloadAll() {
#if NIMONSPOLY_ENABLE_RAYLIB
    for (auto& [key, tex] : textures_) {
        if (tex.id > 0) {
            UnloadTexture(tex);
        }
        (void)key;
    }
    textures_.clear();

    for (auto& [key, font] : fonts_) {
        if (font.texture.id > 0) {
            UnloadFont(font);
        }
        (void)key;
    }
    fonts_.clear();
#endif
    loaded_ = false;
}

#if NIMONSPOLY_ENABLE_RAYLIB
namespace {
    bool textureLoaded(const Texture2D& tex) {
        return tex.id > 0;
    }

    bool fontLoaded(const Font& font) {
        return font.texture.id > 0;
    }

    Font fallbackFont() {
        return GetFontDefault();
    }
}

void AssetManager::loadFonts() {
    struct FontEntry {
        const char* key;
        const char* path;
    };

    const FontEntry entries[] = {
        {"bold",      "assets/fonts/GemunuLibre/GemunuLibre-Bold.ttf"},
        {"extrabold", "assets/fonts/GemunuLibre/GemunuLibre-ExtraBold.ttf"},
        {"semibold",  "assets/fonts/GemunuLibre/GemunuLibre-SemiBold.ttf"},
        {"regular",   "assets/fonts/GemunuLibre/GemunuLibre-Regular.ttf"},
        {"title",     "assets/fonts/Hahmlet/hahmlet-bold.ttf"},
    };

    for (const auto& entry : entries) {
        Font font = LoadFontEx(entry.path, 96, nullptr, 0);
        if (fontLoaded(font)) {
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
            fonts_.emplace(entry.key, font);
        } else {
            std::cerr << "[AssetManager] Font not found: " << entry.path << "\n";
        }
    }
}

const Font& AssetManager::font(const std::string& key) {
    auto it = fonts_.find(key);
    if (it != fonts_.end()) {
        return it->second;
    }

    if (!fonts_.empty()) {
        return fonts_.begin()->second;
    }

    static Font fallback = fallbackFont();
    return fallback;
}

const Texture2D* AssetManager::texture(const std::string& path) {
    auto it = textures_.find(path);
    if (it != textures_.end()) {
        return &it->second;
    }

    Texture2D tex = LoadTexture(path.c_str());
    if (!textureLoaded(tex)) {
        return nullptr;
    }

    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    auto [inserted, ok] = textures_.emplace(path, tex);
    (void)ok;
    return &inserted->second;
}
#endif
