#pragma once

#if NIMONSPOLY_ENABLE_RAYLIB
#include <string>
#include <vector>

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"
#include "ui/RaylibCompat.hpp"

namespace gui::draw {
    RaylibColor tileColor(::Color c);
    RaylibColor tokenColor(int playerIndex);

    Vector2 rectCenter(const Rectangle& rect);
    Rectangle centeredRect(float centerX, float centerY, float width, float height);

    void drawGlobeBackground(int screenW, int screenH);
    void drawGameBackground(int screenW, int screenH);

    void drawCenteredText(AssetManager& am,
                          const std::string& fontKey,
                          const std::string& str,
                          float fontSize,
                          RaylibColor color,
                          float y);

    void drawMenuButton(AssetManager& am,
                        const std::string& label,
                        Rectangle rect,
                        bool hovered,
                        bool selected = false);

    void drawSprite(const Texture2D* tex, Rectangle dest, RaylibColor tint = RL_WHITE);
    void drawSpriteCover(const Texture2D* tex, Rectangle dest, RaylibColor tint = RL_WHITE);
    void drawSpriteCoverScreen(const Texture2D* tex, RaylibColor tint = RL_WHITE);

    void drawLabel(AssetManager& am,
                   const std::string& fontKey,
                   const std::string& str,
                   float fontSize,
                   RaylibColor color,
                   Vector2 pos);

    std::vector<std::string> wrapText(AssetManager& am,
                                      const std::string& fontKey,
                                      const std::string& text,
                                      float fontSize,
                                      float maxWidth);

    void drawPanel(Rectangle rect,
                   RaylibColor fill,
                   RaylibColor outline = RaylibColor{255, 255, 255, 18});
}
#endif
