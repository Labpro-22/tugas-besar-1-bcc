#include "TileRenderer.hpp"

#if NIMONSPOLY_ENABLE_RAYLIB

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "ui/AssetManager.hpp"

namespace gui::tile {
namespace {
    constexpr float kTextSpacing = 0.f;
    constexpr float kCornerRatio = 1.35f;

    RaylibColor makeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        return RaylibColor{r, g, b, a};
    }

    bool isPropertyType(TileType type) {
        return type == TileType::STREET || type == TileType::RAILROAD || type == TileType::UTILITY;
    }

    int quarterTurns(BoardSide side) {
        switch (side) {
            case BoardSide::BOTTOM: return 0;
            case BoardSide::RIGHT: return 1;
            case BoardSide::TOP: return 2;
            case BoardSide::LEFT: return 3;
        }
        return 0;
    }

    float textRotation(BoardSide side) {
        return static_cast<float>(quarterTurns(side) * 90);
    }

    Vector2 logicalSize(const Rectangle& bounds, BoardSide side) {
        if (side == BoardSide::LEFT || side == BoardSide::RIGHT) {
            return Vector2{bounds.height, bounds.width};
        }
        return Vector2{bounds.width, bounds.height};
    }

    Vector2 mapLogicalPoint(Vector2 point, const Rectangle& bounds, BoardSide side) {
        switch (quarterTurns(side)) {
            case 0:
                return Vector2{bounds.x + point.x, bounds.y + point.y};
            case 1:
                return Vector2{bounds.x + bounds.width - point.y, bounds.y + point.x};
            case 2:
                return Vector2{bounds.x + bounds.width - point.x, bounds.y + bounds.height - point.y};
            case 3:
                return Vector2{bounds.x + point.y, bounds.y + bounds.height - point.x};
            default:
                return Vector2{bounds.x + point.x, bounds.y + point.y};
        }
    }

    Rectangle mapLogicalRect(const Rectangle& rect, const Rectangle& bounds, BoardSide side) {
        switch (quarterTurns(side)) {
            case 0:
                return Rectangle{bounds.x + rect.x, bounds.y + rect.y, rect.width, rect.height};
            case 1:
                return Rectangle{
                    bounds.x + bounds.width - (rect.y + rect.height),
                    bounds.y + rect.x,
                    rect.height,
                    rect.width,
                };
            case 2:
                return Rectangle{
                    bounds.x + bounds.width - (rect.x + rect.width),
                    bounds.y + bounds.height - (rect.y + rect.height),
                    rect.width,
                    rect.height,
                };
            case 3:
                return Rectangle{
                    bounds.x + rect.y,
                    bounds.y + bounds.height - (rect.x + rect.width),
                    rect.height,
                    rect.width,
                };
            default:
                return Rectangle{bounds.x + rect.x, bounds.y + rect.y, rect.width, rect.height};
        }
    }

    Vector2 measureText(const Font& font, const std::string& text, float fontSize) {
        return MeasureTextEx(font, text.c_str(), fontSize, kTextSpacing);
    }

    std::string fitText(const Font& font, const std::string& text, float fontSize, float maxWidth) {
        if (text.empty() || measureText(font, text, fontSize).x <= maxWidth) {
            return text;
        }

        std::string clipped = text;
        while (!clipped.empty() && measureText(font, clipped + "...", fontSize).x > maxWidth) {
            clipped.pop_back();
        }
        return clipped.empty() ? text.substr(0, 1) : clipped + "...";
    }

    std::vector<std::string> wrapText(const Font& font,
                                      const std::string& text,
                                      float fontSize,
                                      float maxWidth,
                                      int maxLines) {
        std::vector<std::string> lines;
        if (text.empty() || maxLines <= 0) {
            return lines;
        }

        std::string current;
        std::string word;
        for (size_t i = 0; i <= text.size(); ++i) {
            const bool atEnd = i == text.size();
            const char ch = atEnd ? ' ' : text[i];
            if (ch == ' ' || ch == '\n' || ch == '\t' || atEnd) {
                if (!word.empty()) {
                    const std::string candidate = current.empty() ? word : current + " " + word;
                    if (!current.empty() && measureText(font, candidate, fontSize).x > maxWidth) {
                        lines.push_back(current);
                        current = fitText(font, word, fontSize, maxWidth);
                    } else if (current.empty() && measureText(font, word, fontSize).x > maxWidth) {
                        lines.push_back(fitText(font, word, fontSize, maxWidth));
                        current.clear();
                    } else {
                        current = candidate;
                    }
                    word.clear();
                }
            } else {
                word.push_back(ch);
            }
        }

        if (!current.empty()) {
            lines.push_back(current);
        }
        if (lines.empty()) {
            lines.push_back(fitText(font, text, fontSize, maxWidth));
        }
        if (static_cast<int>(lines.size()) > maxLines) {
            lines.resize(static_cast<size_t>(maxLines));
            lines.back() = fitText(font, lines.back(), fontSize, maxWidth);
        }
        return lines;
    }

    void drawCenteredRotatedText(const Font& font,
                                 const std::string& text,
                                 float fontSize,
                                 RaylibColor color,
                                 Vector2 logicalCenter,
                                 const Rectangle& bounds,
                                 BoardSide side) {
        const Vector2 textSize = measureText(font, text, fontSize);
        DrawTextPro(font,
                    text.c_str(),
                    mapLogicalPoint(logicalCenter, bounds, side),
                    Vector2{textSize.x * 0.5f, textSize.y * 0.5f},
                    textRotation(side),
                    fontSize,
                    kTextSpacing,
                    color);
    }

    void drawTextBlock(const Font& font,
                       const std::string& text,
                       float fontSize,
                       RaylibColor color,
                       const Rectangle& logicalRect,
                       const Rectangle& bounds,
                       BoardSide side,
                       int maxLines) {
        const auto lines = wrapText(font, text, fontSize, std::max(12.f, logicalRect.width), maxLines);
        if (lines.empty()) {
            return;
        }

        const float lineHeight = fontSize * 0.95f;
        const float blockHeight = lineHeight * static_cast<float>(lines.size());
        float centerY = logicalRect.y + std::max(0.f, (logicalRect.height - blockHeight) * 0.5f) + lineHeight * 0.5f;
        for (const std::string& line : lines) {
            drawCenteredRotatedText(font,
                                    line,
                                    fontSize,
                                    color,
                                    Vector2{logicalRect.x + logicalRect.width * 0.5f, centerY},
                                    bounds,
                                    side);
            centerY += lineHeight;
        }
    }

    std::string moneyLabel(int amount) {
        if (amount <= 0) {
            return "";
        }
        return "M" + std::to_string(amount);
    }

    bool isCornerType(TileType type) {
        return type == TileType::GO || type == TileType::JAIL ||
               type == TileType::FREE_PARKING || type == TileType::GO_TO_JAIL;
    }

    RaylibColor propertyGroupColor(const TileData& tile) {
        switch (tile.color) {
            case ::Color::BROWN: return makeColor(0x8b, 0x5a, 0x2b);
            case ::Color::LIGHT_BLUE: return makeColor(0x47, 0xc7, 0xff);
            case ::Color::PINK: return makeColor(0xef, 0x5a, 0xa1);
            case ::Color::ORANGE: return makeColor(0xff, 0x91, 0x2f);
            case ::Color::RED: return makeColor(0xe4, 0x47, 0x47);
            case ::Color::YELLOW: return makeColor(0xf7, 0xcf, 0x3d);
            case ::Color::GREEN: return makeColor(0x32, 0xb8, 0x79);
            case ::Color::DARK_BLUE: return makeColor(0x29, 0x58, 0xd7);
            case ::Color::GRAY: return makeColor(0x7d, 0x89, 0x99);
            default: return makeColor(0x6a, 0x74, 0x84);
        }
    }

    RaylibColor typeAccent(const TileData& tile) {
        if (tile.type == TileType::STREET) {
            return propertyGroupColor(tile);
        }

        switch (tile.type) {
            case TileType::RAILROAD: return makeColor(0x58, 0x63, 0x76);
            case TileType::UTILITY: return makeColor(0x38, 0x95, 0xd8);
            case TileType::CHANCE: return makeColor(0xf2, 0x93, 0x37);
            case TileType::COMMUNITY_CHEST: return makeColor(0x2f, 0xc1, 0xbe);
            case TileType::FESTIVAL: return makeColor(0xd4, 0x5b, 0xff);
            case TileType::TAX_PPH:
            case TileType::TAX_PBM: return makeColor(0xec, 0x5d, 0x5d);
            case TileType::GO: return makeColor(0x2b, 0xb9, 0x6c);
            case TileType::JAIL: return makeColor(0xff, 0xb4, 0x54);
            case TileType::FREE_PARKING: return makeColor(0x31, 0xa7, 0xf3);
            case TileType::GO_TO_JAIL: return makeColor(0xb6, 0x4b, 0x4b);
            default: return makeColor(0x70, 0x7d, 0x90);
        }
    }

    RaylibColor tileFillColor(const TileData& tile) {
        if (isPropertyType(tile.type)) {
            return makeColor(0xf8, 0xf6, 0xf1);
        }

        const RaylibColor accent = typeAccent(tile);
        return makeColor(
            static_cast<unsigned char>(std::min(255, accent.r + 175)),
            static_cast<unsigned char>(std::min(255, accent.g + 175)),
            static_cast<unsigned char>(std::min(255, accent.b + 175)),
            255);
    }

    RaylibColor tileBorderColor(const TileData& tile) {
        const RaylibColor accent = typeAccent(tile);
        return makeColor(
            static_cast<unsigned char>(std::max(35, static_cast<int>(accent.r) - 55)),
            static_cast<unsigned char>(std::max(35, static_cast<int>(accent.g) - 55)),
            static_cast<unsigned char>(std::max(35, static_cast<int>(accent.b) - 55)),
            255);
    }

    Rectangle insetRect(const Rectangle& rect, float amount) {
        return Rectangle{
            rect.x + amount,
            rect.y + amount,
            std::max(0.f, rect.width - amount * 2.f),
            std::max(0.f, rect.height - amount * 2.f),
        };
    }

    void drawRailroadIcon(const Rectangle& rect, RaylibColor accent) {
        const float trackY1 = rect.y + rect.height * 0.32f;
        const float trackY2 = rect.y + rect.height * 0.68f;
        DrawLineEx(Vector2{rect.x + rect.width * 0.20f, trackY1},
                   Vector2{rect.x + rect.width * 0.80f, trackY1},
                   2.4f,
                   accent);
        DrawLineEx(Vector2{rect.x + rect.width * 0.20f, trackY2},
                   Vector2{rect.x + rect.width * 0.80f, trackY2},
                   2.4f,
                   accent);
        for (int i = 0; i < 5; ++i) {
            const float x = rect.x + rect.width * (0.24f + 0.14f * static_cast<float>(i));
            DrawLineEx(Vector2{x, rect.y + rect.height * 0.24f},
                       Vector2{x, rect.y + rect.height * 0.76f},
                       2.2f,
                       makeColor(accent.r, accent.g, accent.b, 140));
        }
    }

    void drawUtilityIcon(const Rectangle& rect, RaylibColor accent) {
        const float cx = rect.x + rect.width * 0.5f;
        const float cy = rect.y + rect.height * 0.5f;
        const float radius = std::min(rect.width, rect.height) * 0.42f;
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), radius, accent);

        const Vector2 bolt[6] = {
            Vector2{cx - radius * 0.12f, cy - radius * 0.72f},
            Vector2{cx + radius * 0.08f, cy - radius * 0.18f},
            Vector2{cx - radius * 0.02f, cy - radius * 0.18f},
            Vector2{cx + radius * 0.16f, cy + radius * 0.72f},
            Vector2{cx - radius * 0.12f, cy + radius * 0.14f},
            Vector2{cx + radius * 0.02f, cy + radius * 0.14f},
        };
        for (int i = 0; i < 5; ++i) {
            DrawLineEx(bolt[i], bolt[i + 1], 2.6f, accent);
        }
    }

    void drawCardIcon(const Rectangle& rect, RaylibColor accent, bool community) {
        const Rectangle back{
            rect.x + rect.width * 0.18f,
            rect.y + rect.height * 0.14f,
            rect.width * 0.48f,
            rect.height * 0.58f,
        };
        const Rectangle front{
            rect.x + rect.width * 0.30f,
            rect.y + rect.height * 0.26f,
            rect.width * 0.48f,
            rect.height * 0.58f,
        };
        DrawRectangleRounded(back, 0.16f, 6, makeColor(accent.r, accent.g, accent.b, 60));
        DrawRectangleRounded(front, 0.16f, 6, makeColor(accent.r, accent.g, accent.b, 90));
        DrawRectangleLinesEx(front, 2.f, accent);

        if (community) {
            DrawRectangleRec(Rectangle{
                front.x + front.width * 0.20f,
                front.y + front.height * 0.20f,
                front.width * 0.18f,
                front.height * 0.18f,
            }, accent);
            DrawLineEx(Vector2{front.x + front.width * 0.20f, front.y + front.height * 0.56f},
                       Vector2{front.x + front.width * 0.80f, front.y + front.height * 0.56f},
                       2.f,
                       accent);
            DrawLineEx(Vector2{front.x + front.width * 0.20f, front.y + front.height * 0.72f},
                       Vector2{front.x + front.width * 0.68f, front.y + front.height * 0.72f},
                       2.f,
                       accent);
        } else {
            const Font& font = AssetManager::get().font("extrabold");
            const std::string symbol = "?";
            const float size = front.height * 0.58f;
            const Vector2 textSize = measureText(font, symbol, size);
            DrawTextEx(font,
                       symbol.c_str(),
                       Vector2{
                           front.x + front.width * 0.5f - textSize.x * 0.5f,
                           front.y + front.height * 0.5f - textSize.y * 0.5f,
                       },
                       size,
                       kTextSpacing,
                       accent);
        }
    }

    void drawFestivalIcon(const Rectangle& rect, RaylibColor accent) {
        const Vector2 center{rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f};
        const float radius = std::min(rect.width, rect.height) * 0.12f;
        DrawCircleV(center, radius, accent);
        for (int i = 0; i < 8; ++i) {
            const float angle = static_cast<float>(i) * 45.f * DEG2RAD;
            const Vector2 outer{
                center.x + std::cos(angle) * rect.width * 0.34f,
                center.y + std::sin(angle) * rect.height * 0.34f,
            };
            DrawLineEx(center, outer, 2.2f, accent);
        }
    }

    void drawTaxIcon(const Rectangle& rect, RaylibColor accent) {
        const float radius = std::min(rect.width, rect.height) * 0.10f;
        const Vector2 top{rect.x + rect.width * 0.65f, rect.y + rect.height * 0.30f};
        const Vector2 bottom{rect.x + rect.width * 0.35f, rect.y + rect.height * 0.70f};
        DrawCircleLines(static_cast<int>(top.x), static_cast<int>(top.y), radius, accent);
        DrawCircleLines(static_cast<int>(bottom.x), static_cast<int>(bottom.y), radius, accent);
        DrawLineEx(Vector2{rect.x + rect.width * 0.28f, rect.y + rect.height * 0.78f},
                   Vector2{rect.x + rect.width * 0.72f, rect.y + rect.height * 0.22f},
                   2.4f,
                   accent);
    }

    void drawArrowIcon(const Rectangle& rect, RaylibColor accent) {
        const float midY = rect.y + rect.height * 0.50f;
        const float startX = rect.x + rect.width * 0.18f;
        const float endX = rect.x + rect.width * 0.78f;
        DrawLineEx(Vector2{startX, midY}, Vector2{endX, midY}, 3.f, accent);
        DrawLineEx(Vector2{endX, midY},
                   Vector2{rect.x + rect.width * 0.60f, rect.y + rect.height * 0.28f},
                   3.f,
                   accent);
        DrawLineEx(Vector2{endX, midY},
                   Vector2{rect.x + rect.width * 0.60f, rect.y + rect.height * 0.72f},
                   3.f,
                   accent);
    }

    void drawParkingIcon(const Rectangle& rect, RaylibColor accent) {
        const float radius = std::min(rect.width, rect.height) * 0.34f;
        const Vector2 center{rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f};
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, accent);
        const Font& font = AssetManager::get().font("extrabold");
        const std::string symbol = "P";
        const float size = radius * 1.35f;
        const Vector2 textSize = measureText(font, symbol, size);
        DrawTextEx(font,
                   symbol.c_str(),
                   Vector2{center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f},
                   size,
                   kTextSpacing,
                   accent);
    }

    void drawJailIcon(const Rectangle& rect, RaylibColor accent) {
        const Rectangle body{
            rect.x + rect.width * 0.22f,
            rect.y + rect.height * 0.18f,
            rect.width * 0.56f,
            rect.height * 0.64f,
        };
        DrawRectangleLinesEx(body, 2.f, accent);
        for (int i = 1; i <= 3; ++i) {
            const float x = body.x + body.width * (0.20f * static_cast<float>(i));
            DrawLineEx(Vector2{x, body.y}, Vector2{x, body.y + body.height}, 2.1f, accent);
        }
    }

    void drawSpecialIcon(const TileData& tile, const Rectangle& rect) {
        const RaylibColor accent = typeAccent(tile);
        switch (tile.type) {
            case TileType::RAILROAD:
                drawRailroadIcon(rect, accent);
                break;
            case TileType::UTILITY:
                drawUtilityIcon(rect, accent);
                break;
            case TileType::CHANCE:
                drawCardIcon(rect, accent, false);
                break;
            case TileType::COMMUNITY_CHEST:
                drawCardIcon(rect, accent, true);
                break;
            case TileType::FESTIVAL:
                drawFestivalIcon(rect, accent);
                break;
            case TileType::TAX_PPH:
            case TileType::TAX_PBM:
                drawTaxIcon(rect, accent);
                break;
            case TileType::GO:
                drawArrowIcon(rect, accent);
                break;
            case TileType::FREE_PARKING:
                drawParkingIcon(rect, accent);
                break;
            case TileType::JAIL:
            case TileType::GO_TO_JAIL:
                drawJailIcon(rect, accent);
                if (tile.type == TileType::GO_TO_JAIL) {
                    drawArrowIcon(Rectangle{
                        rect.x + rect.width * 0.18f,
                        rect.y + rect.height * 0.55f,
                        rect.width * 0.48f,
                        rect.height * 0.28f,
                    }, accent);
                }
                break;
            default:
                break;
        }
    }

    void drawStatusBadge(const TileData& tile,
                         const Rectangle& bounds,
                         BoardSide side,
                         const Rectangle& logicalRect) {
        if (!tile.isOwnable || !tile.isMortgaged) {
            return;
        }

        const Rectangle badge = mapLogicalRect(logicalRect, bounds, side);
        DrawRectangleRounded(badge, 0.35f, 6, makeColor(0x2b, 0x2c, 0x34, 220));
        DrawRectangleLinesEx(badge, 1.4f, makeColor(0xff, 0x75, 0x75));

        const Font& font = AssetManager::get().font("bold");
        const std::string text = "M";
        const float size = std::min(badge.width, badge.height) * 0.54f;
        const Vector2 textSize = measureText(font, text, size);
        DrawTextEx(font,
                   text.c_str(),
                   Vector2{
                       badge.x + badge.width * 0.5f - textSize.x * 0.5f,
                       badge.y + badge.height * 0.5f - textSize.y * 0.5f,
                   },
                   size,
                   kTextSpacing,
                   makeColor(0xff, 0xc7, 0xc7));
    }

    void drawHouseMarkers(const TileData& tile,
                          const Rectangle& bounds,
                          BoardSide side,
                          const Rectangle& logicalRect) {
        if (!tile.isOwnable || tile.buildingLevel <= 0) {
            return;
        }

        const int markers = tile.hasHotel ? 1 : std::min(4, tile.houseCount);
        const float gap = 4.f;

        if (tile.hasHotel) {
            const Rectangle hotel = mapLogicalRect(logicalRect, bounds, side);
            DrawRectangleRounded(hotel, 0.18f, 6, makeColor(0xff, 0x82, 0x78));
            DrawRectangleLinesEx(hotel, 1.2f, makeColor(0x91, 0x2d, 0x2d));
            return;
        }

        const float markerWidth =
            std::min(12.f, (logicalRect.width - gap * static_cast<float>(markers - 1)) / static_cast<float>(markers));
        const float totalWidth = markerWidth * static_cast<float>(markers) + gap * static_cast<float>(markers - 1);
        float x = logicalRect.x + logicalRect.width * 0.5f - totalWidth * 0.5f;
        for (int i = 0; i < markers; ++i) {
            const Rectangle house = mapLogicalRect(Rectangle{x, logicalRect.y, markerWidth, logicalRect.height}, bounds, side);
            DrawRectangleRounded(house, 0.18f, 4, makeColor(0x34, 0xc7, 0x74));
            DrawRectangleLinesEx(house, 1.f, makeColor(0x13, 0x61, 0x35));
            x += markerWidth + gap;
        }
    }
}

int boardEdgeTileCount(int totalTiles) {
    if (totalTiles < 4) {
        return totalTiles > 0 ? totalTiles : 1;
    }
    return std::max(1, totalTiles / 4);
}

BoardSide boardSideForIndex(int tileIndex, int totalTiles) {
    const int edgeCount = boardEdgeTileCount(totalTiles);
    const int index = totalTiles > 0 ? ((tileIndex % totalTiles) + totalTiles) % totalTiles : 0;
    if (index < edgeCount) {
        return BoardSide::BOTTOM;
    }
    if (index < edgeCount * 2) {
        return BoardSide::LEFT;
    }
    if (index < edgeCount * 3) {
        return BoardSide::TOP;
    }
    return BoardSide::RIGHT;
}

Rectangle boardTileBounds(int tileIndex, Rectangle boardBounds, int totalTiles) {
    const int edgeCount = boardEdgeTileCount(totalTiles);
    if (totalTiles <= 0 || edgeCount <= 0) {
        return boardBounds;
    }

    const int index = ((tileIndex % totalTiles) + totalTiles) % totalTiles;
    const float boardSize = std::min(boardBounds.width, boardBounds.height);
    const float edgeDepth = (edgeCount > 1)
        ? boardSize / ((static_cast<float>(edgeCount) - 1.f) + kCornerRatio * 2.f)
        : boardSize;
    const float corner = (edgeCount > 1) ? edgeDepth * kCornerRatio : boardSize;
    const float edgeLength = (edgeCount > 1) ? edgeDepth : boardSize;
    const float x = boardBounds.x;
    const float y = boardBounds.y;
    const float right = x + boardSize;
    const float bottom = y + boardSize;

    if (index == 0) {
        return Rectangle{right - corner, bottom - corner, corner, corner};
    }
    if (index < edgeCount) {
        const int step = index - 1;
        return Rectangle{
            right - corner - edgeLength * static_cast<float>(step + 1),
            bottom - corner,
            edgeLength,
            corner,
        };
    }
    if (index == edgeCount) {
        return Rectangle{x, bottom - corner, corner, corner};
    }
    if (index < edgeCount * 2) {
        const int step = index - edgeCount - 1;
        return Rectangle{
            x,
            bottom - corner - edgeLength * static_cast<float>(step + 1),
            corner,
            edgeLength,
        };
    }
    if (index == edgeCount * 2) {
        return Rectangle{x, y, corner, corner};
    }
    if (index < edgeCount * 3) {
        const int step = index - edgeCount * 2 - 1;
        return Rectangle{
            x + corner + edgeLength * static_cast<float>(step),
            y,
            edgeLength,
            corner,
        };
    }
    if (index == edgeCount * 3) {
        return Rectangle{right - corner, y, corner, corner};
    }

    const int step = index - edgeCount * 3 - 1;
    return Rectangle{
        right - corner,
        y + corner + edgeLength * static_cast<float>(step),
        corner,
        edgeLength,
    };
}

void drawPropertyTile(const TileData& tile, Rectangle bounds, BoardSide side) {
    AssetManager& am = AssetManager::get();
    const Font& titleFont = am.font("semibold");
    const Font& labelFont = am.font("regular");
    const RaylibColor fill = tileFillColor(tile);
    const RaylibColor border = tileBorderColor(tile);
    const RaylibColor accent = typeAccent(tile);
    const Vector2 logical = logicalSize(bounds, side);
    const float pad = std::clamp(std::min(logical.x, logical.y) * 0.10f, 6.f, 16.f);
    const float stripDepth = std::clamp(logical.y * 0.18f, 10.f, 24.f);
    const float codeHeight = std::clamp(logical.y * 0.16f, 10.f, 20.f);
    const Rectangle inner = insetRect(bounds, 1.2f);
    const float bodyTop = pad + codeHeight + 4.f;
    const float bodyHeight = std::max(18.f, logical.y - stripDepth - bodyTop - pad * 0.7f);

    DrawRectangleRec(bounds, makeColor(0x07, 0x0d, 0x18, 180));
    DrawRectangleRounded(inner, 0.08f, 8, fill);
    DrawRectangleLinesEx(inner, 1.4f, border);

    const Rectangle strip = mapLogicalRect(Rectangle{0.f, logical.y - stripDepth, logical.x, stripDepth}, inner, side);
    DrawRectangleRec(strip, accent);

    const Rectangle codeRectLocal{
        pad,
        pad,
        std::max(28.f, logical.x * 0.36f),
        codeHeight,
    };
    const Rectangle codeRect = mapLogicalRect(codeRectLocal, inner, side);
    DrawRectangleRounded(codeRect, 0.22f, 6, makeColor(accent.r, accent.g, accent.b, 30));
    DrawRectangleLinesEx(codeRect, 1.f, makeColor(accent.r, accent.g, accent.b, 120));
    drawTextBlock(labelFont,
                  tile.code,
                  std::clamp(codeHeight * 0.65f, 9.f, 16.f),
                  border,
                  codeRectLocal,
                  inner,
                  side,
                  1);

    if (tile.type == TileType::RAILROAD || tile.type == TileType::UTILITY) {
        const Rectangle iconRect = mapLogicalRect(Rectangle{
            logical.x * 0.25f,
            bodyTop,
            logical.x * 0.50f,
            std::min(bodyHeight * 0.34f, logical.y * 0.28f),
        }, inner, side);
        drawSpecialIcon(tile, iconRect);
    }

    const float titleFontSize = std::clamp(std::min(logical.x, logical.y) * 0.16f, 10.f, 18.f);
    const float subtitleFontSize = std::clamp(titleFontSize * 0.72f, 8.f, 13.f);
    const float priceFontSize = std::clamp(titleFontSize * 0.78f, 8.f, 14.f);

    drawTextBlock(titleFont,
                  tile.name,
                  titleFontSize,
                  makeColor(0x1f, 0x28, 0x37),
                  Rectangle{
                      pad,
                      bodyTop + (tile.type == TileType::STREET ? 0.f : logical.y * 0.12f),
                      logical.x - pad * 2.f,
                      bodyHeight * 0.48f,
                  },
                  inner,
                  side,
                  tile.type == TileType::STREET ? 2 : 1);

    if (!tile.subtitle.empty()) {
        drawTextBlock(labelFont,
                      tile.subtitle,
                      subtitleFontSize,
                      makeColor(0x6f, 0x7a, 0x89),
                      Rectangle{
                          pad,
                          bodyTop + bodyHeight * 0.42f,
                          logical.x - pad * 2.f,
                          bodyHeight * 0.18f,
                      },
                      inner,
                      side,
                      1);
    }

    const std::string price = moneyLabel(tile.price);
    if (!price.empty()) {
        drawTextBlock(titleFont,
                      price,
                      priceFontSize,
                      makeColor(0x15, 0x53, 0x34),
                      Rectangle{
                          logical.x * 0.18f,
                          logical.y - stripDepth - pad - priceFontSize * 1.3f,
                          logical.x * 0.64f,
                          priceFontSize * 1.4f,
                      },
                      inner,
                      side,
                      1);
    }

    drawHouseMarkers(tile,
                     inner,
                     side,
                     Rectangle{
                         logical.x * 0.18f,
                         logical.y - stripDepth - pad * 1.25f,
                         logical.x * 0.64f,
                         std::clamp(logical.y * 0.10f, 7.f, 14.f),
                     });

    drawStatusBadge(tile,
                    inner,
                    side,
                    Rectangle{
                        logical.x - pad - codeHeight,
                        pad,
                        codeHeight,
                        codeHeight,
                    });
}

void drawSpecialTile(const TileData& tile, Rectangle bounds, BoardSide side) {
    AssetManager& am = AssetManager::get();
    const Font& titleFont = am.font("semibold");
    const Font& labelFont = am.font("regular");
    const RaylibColor fill = tileFillColor(tile);
    const RaylibColor border = tileBorderColor(tile);
    const RaylibColor accent = typeAccent(tile);
    const Vector2 logical = logicalSize(bounds, side);
    const float pad = std::clamp(std::min(logical.x, logical.y) * 0.10f, 7.f, 18.f);
    const float titleFontSize = std::clamp(std::min(logical.x, logical.y) * (isCornerType(tile.type) ? 0.18f : 0.15f), 10.f, 22.f);
    const float subtitleFontSize = std::clamp(titleFontSize * 0.68f, 8.f, 13.f);
    const Rectangle inner = insetRect(bounds, 1.2f);
    const float badgeHeight = std::clamp(logical.y * 0.16f, 10.f, 24.f);

    DrawRectangleRec(bounds, makeColor(0x07, 0x0d, 0x18, 180));
    DrawRectangleRounded(inner, 0.08f, 8, fill);
    DrawRectangleLinesEx(inner, 1.4f, border);

    const Rectangle badge = mapLogicalRect(Rectangle{pad, pad, logical.x - pad * 2.f, badgeHeight}, inner, side);
    DrawRectangleRounded(badge, 0.22f, 6, makeColor(accent.r, accent.g, accent.b, 26));
    DrawRectangleLinesEx(badge, 1.f, makeColor(accent.r, accent.g, accent.b, 100));
    drawTextBlock(labelFont,
                  tile.code,
                  std::clamp(badgeHeight * 0.62f, 8.f, 15.f),
                  accent,
                  Rectangle{pad, pad, logical.x - pad * 2.f, badgeHeight},
                  inner,
                  side,
                  1);

    const Rectangle iconRect = mapLogicalRect(Rectangle{
        logical.x * 0.18f,
        pad + badgeHeight + pad * 0.5f,
        logical.x * 0.64f,
        logical.y * (isCornerType(tile.type) ? 0.34f : 0.28f),
    }, inner, side);
    drawSpecialIcon(tile, iconRect);

    drawTextBlock(titleFont,
                  tile.name,
                  titleFontSize,
                  makeColor(0x1e, 0x2b, 0x3d),
                  Rectangle{
                      pad,
                      logical.y * (isCornerType(tile.type) ? 0.52f : 0.48f),
                      logical.x - pad * 2.f,
                      logical.y * 0.22f,
                  },
                  inner,
                  side,
                  isCornerType(tile.type) ? 2 : 1);

    if (!tile.subtitle.empty()) {
        drawTextBlock(labelFont,
                      tile.subtitle,
                      subtitleFontSize,
                      makeColor(0x5f, 0x6e, 0x83),
                      Rectangle{
                          pad,
                          logical.y * 0.76f,
                          logical.x - pad * 2.f,
                          logical.y * 0.12f,
                      },
                      inner,
                      side,
                      1);
    }

    if (tile.type == TileType::TAX_PPH || tile.type == TileType::TAX_PBM) {
        const std::string price = moneyLabel(tile.price);
        if (!price.empty()) {
            drawTextBlock(titleFont,
                          price,
                          subtitleFontSize,
                          makeColor(0x8e, 0x2d, 0x2d),
                          Rectangle{
                              logical.x * 0.20f,
                              logical.y * 0.86f,
                              logical.x * 0.60f,
                              logical.y * 0.08f,
                          },
                          inner,
                          side,
                          1);
        }
    }
}

void drawTile(const TileData& tile, Rectangle bounds, BoardSide side) {
    if (isPropertyType(tile.type)) {
        drawPropertyTile(tile, bounds, side);
        return;
    }
    drawSpecialTile(tile, bounds, side);
}

}  // namespace gui::tile

#endif
