#include "GUIViewDraw.hpp"

#if NIMONSPOLY_ENABLE_RAYLIB
#include <algorithm>
#include <array>
#include <sstream>

namespace gui::draw {
namespace {
    constexpr float kTextSpacing = 0.f;

    RaylibColor makeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        return RaylibColor{r, g, b, a};
    }

    Vector2 measureText(const Font& font, const std::string& text, float fontSize) {
        return MeasureTextEx(font, text.c_str(), fontSize, kTextSpacing);
    }
}

RaylibColor tileColor(::Color c) {
    switch (c) {
        case ::Color::BROWN:      return makeColor(0xda, 0xcc, 0xc1);
        case ::Color::LIGHT_BLUE: return makeColor(0x2e, 0xc7, 0xff);
        case ::Color::PINK:       return makeColor(0xee, 0x2a, 0x89);
        case ::Color::ORANGE:     return makeColor(0xff, 0x77, 0x00);
        case ::Color::RED:        return makeColor(0xbe, 0x17, 0x1a);
        case ::Color::YELLOW:     return makeColor(0xff, 0xf2, 0x00);
        case ::Color::GREEN:      return makeColor(0x00, 0x93, 0x64);
        case ::Color::DARK_BLUE:  return makeColor(0x12, 0x48, 0x72);
        case ::Color::GRAY:       return makeColor(0xa0, 0xa0, 0xa0);
        default:                  return makeColor(0x37, 0x3c, 0x46);
    }
}

RaylibColor tokenColor(int playerIndex) {
    constexpr std::array<RaylibColor, 4> colors{
        RaylibColor{0xe3, 0x4b, 0x4b, 255},
        RaylibColor{0xf4, 0xd3, 0x1c, 255},
        RaylibColor{0x41, 0x8d, 0xff, 255},
        RaylibColor{0x32, 0xc4, 0x71, 255},
    };
    return colors[static_cast<size_t>(playerIndex) % colors.size()];
}

Vector2 rectCenter(const Rectangle& rect) {
    return Vector2{
        rect.x + rect.width * 0.5f,
        rect.y + rect.height * 0.5f,
    };
}

Rectangle centeredRect(float centerX, float centerY, float width, float height) {
    return Rectangle{
        centerX - width * 0.5f,
        centerY - height * 0.5f,
        width,
        height,
    };
}

void drawGlobeBackground(int screenW, int screenH) {
    const float cx = static_cast<float>(screenW) * 0.5f;
    const float cy = static_cast<float>(screenH) * 0.5f;
    const float r = std::min(screenW, screenH) * 0.44f;

    DrawCircleGradient(Vector2{cx, cy}, r * 1.35f,
                       makeColor(100, 160, 255, 18), RL_BLANK);
    DrawCircleV(Vector2{cx, cy}, r, makeColor(0x0d, 0x18, 0x2e));
    DrawCircleLinesV(Vector2{cx, cy}, r, makeColor(100, 160, 255, 80));
    DrawCircleLinesV(Vector2{cx, cy}, r * 0.92f, makeColor(120, 180, 255, 40));
}

void drawGameBackground(int screenW, int screenH) {
    DrawRectangleGradientV(0, 0, screenW, screenH, makeColor(0xd8, 0xde, 0xe6), makeColor(0xc7, 0xcf, 0xda));
    DrawCircleGradient(Vector2{screenW * 0.52f, screenH * 0.46f},
                       std::min(screenW, screenH) * 0.42f,
                       makeColor(255, 255, 255, 55), RL_BLANK);
}

void drawCenteredText(AssetManager& am,
                      const std::string& fontKey,
                      const std::string& str,
                      float fontSize,
                      RaylibColor color,
                      float y) {
    const Font& font = am.font(fontKey);
    const Vector2 textSize = measureText(font, str, fontSize);
    DrawTextEx(font,
               str.c_str(),
               Vector2{
                   GetScreenWidth() * 0.5f - textSize.x * 0.5f,
                   y - textSize.y * 0.5f,
               },
               fontSize,
               kTextSpacing,
               color);
}

void drawMenuButton(AssetManager& am,
                    const std::string& label,
                    Rectangle rect,
                    bool hovered,
                    bool selected) {
    const RaylibColor fill = selected ? makeColor(255, 255, 255, 210)
                                      : hovered ? makeColor(255, 255, 255, 60)
                                                : makeColor(255, 255, 255, 20);
    const RaylibColor outline = makeColor(255, 255, 255, selected ? 200 : 60);
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 1.f, outline);

    const RaylibColor textColor = (selected || hovered) ? RL_WHITE : makeColor(180, 200, 220);
    const float fontSize = rect.height * 0.52f;
    const Font& font = am.font("bold");
    const Vector2 textSize = measureText(font, label, fontSize);
    DrawTextEx(font,
               label.c_str(),
               Vector2{
                   rect.x + rect.width * 0.5f - textSize.x * 0.5f,
                   rect.y + rect.height * 0.5f - textSize.y * 0.5f,
               },
               fontSize,
               kTextSpacing,
               textColor);
}

void drawSprite(const Texture2D* tex, Rectangle dest, RaylibColor tint) {
    if (!tex || tex->id <= 0) {
        return;
    }
    const Rectangle src{0.f, 0.f, static_cast<float>(tex->width), static_cast<float>(tex->height)};
    DrawTexturePro(*tex, src, dest, Vector2{0.f, 0.f}, 0.f, tint);
}

void drawSpriteCover(const Texture2D* tex, Rectangle dest, RaylibColor tint) {
    if (!tex || tex->id <= 0) {
        return;
    }

    const float texW = static_cast<float>(tex->width);
    const float texH = static_cast<float>(tex->height);
    const float targetAspect = dest.width / dest.height;
    const float textureAspect = texW / texH;

    Rectangle src{};
    if (textureAspect > targetAspect) {
        const float desiredW = texH * targetAspect;
        src = Rectangle{(texW - desiredW) * 0.5f, 0.f, desiredW, texH};
    } else {
        const float desiredH = texW / targetAspect;
        src = Rectangle{0.f, (texH - desiredH) * 0.5f, texW, desiredH};
    }

    DrawTexturePro(*tex, src, dest, Vector2{0.f, 0.f}, 0.f, tint);
}

void drawSpriteCoverScreen(const Texture2D* tex, RaylibColor tint) {
    drawSpriteCover(tex, Rectangle{0.f, 0.f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}, tint);
}

void drawLabel(AssetManager& am,
               const std::string& fontKey,
               const std::string& str,
               float fontSize,
               RaylibColor color,
               Vector2 pos) {
    const Font& font = am.font(fontKey);
    DrawTextEx(font, str.c_str(), pos, fontSize, kTextSpacing, color);
}

std::vector<std::string> wrapText(AssetManager& am,
                                  const std::string& fontKey,
                                  const std::string& text,
                                  float fontSize,
                                  float maxWidth) {
    std::vector<std::string> lines;
    std::string currentLine;
    std::istringstream iss(text);
    std::string word;
    const Font& font = am.font(fontKey);

    while (iss >> word) {
        const std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
        if (measureText(font, candidate, fontSize).x > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine = word;
        } else {
            currentLine = candidate;
        }
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    return lines;
}

void drawPanel(Rectangle rect, RaylibColor fill, RaylibColor outline) {
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 1.f, outline);
}

}  // namespace gui::draw
#endif
