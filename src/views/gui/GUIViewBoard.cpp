#include "ui/GUIView.hpp"

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"

#if NIMONSPOLY_ENABLE_RAYLIB
#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <vector>

#include "GuiMenuLayout.hpp"
#include "components/GUIViewDraw.hpp"
#include "renderers/TileRenderer.hpp"
#endif

namespace {
    [[maybe_unused]] constexpr float LP_W_FRAC = 0.16f;
    [[maybe_unused]] constexpr float RP_W_FRAC = 0.16f;
    [[maybe_unused]] constexpr float BOT_H_FRAC = 0.20f;
}

#if NIMONSPOLY_ENABLE_RAYLIB
namespace {
    const RaylibColor BG_COLOR = gui::menu::makeColor(0x06, 0x09, 0x12);
    const RaylibColor PANEL_BG = gui::menu::makeColor(0x08, 0x0c, 0x14);
    const RaylibColor PANEL_BORDER = gui::menu::makeColor(0x5c, 0xd6, 0xff, 40);
    const RaylibColor ACCENT_CYAN = gui::menu::makeColor(0x5c, 0xd6, 0xff);
    const RaylibColor ACCENT_GOLD = gui::menu::makeColor(0xff, 0xc1, 0x07);
    const RaylibColor TEXT_PRIMARY = gui::menu::makeColor(0xe8, 0xed, 0xf2);
    const RaylibColor TEXT_MUTED = gui::menu::makeColor(0x7a, 0x85, 0x98);
    const RaylibColor TEXT_GREEN = gui::menu::makeColor(0x00, 0xd9, 0x8e);
    const RaylibColor BTN_BG = gui::menu::makeColor(0x12, 0x1a, 0x2e);
    const RaylibColor BTN_BORDER = gui::menu::makeColor(0x2a, 0x3f, 0x5c);

    std::string setupPlayerName(const SetupState& setup, int playerIndex) {
        if (playerIndex >= 0 &&
            playerIndex < static_cast<int>(setup.playerNames.size()) &&
            !setup.playerNames[static_cast<size_t>(playerIndex)].empty()) {
            return setup.playerNames[static_cast<size_t>(playerIndex)];
        }
        return "P" + std::to_string(playerIndex + 1);
    }

    int findSetupPlayerIndex(const SetupState& setup, const std::string& username, int fallbackIndex) {
        for (int i = 0; i < static_cast<int>(setup.playerNames.size()); ++i) {
            if (setupPlayerName(setup, i) == username) {
                return i;
            }
        }
        return fallbackIndex;
    }

    int selectedPlayerColor(const SetupState& setup, const std::string& username, int fallbackIndex) {
        return gui::menu::selectedPlayerColor(
            setup, findSetupPlayerIndex(setup, username, fallbackIndex));
    }

    int selectedPlayerCharacter(const SetupState& setup, const std::string& username, int fallbackIndex) {
        return gui::menu::selectedPlayerCharacter(
            setup, findSetupPlayerIndex(setup, username, fallbackIndex));
    }

    std::string tokenTexturePath(const SetupState& setup, const std::string& username, int fallbackIndex) {
        const int colorIndex = selectedPlayerColor(setup, username, fallbackIndex);
        const int characterIndex = selectedPlayerCharacter(setup, username, fallbackIndex);
        return "assets/components/characters/" +
               std::string(gui::menu::characterKeys()[static_cast<size_t>(characterIndex)]) +
               "_" + gui::menu::setupColorKeys()[static_cast<size_t>(colorIndex)] + ".png";
    }

    void drawTextCentered(const Font& font,
                          const std::string& text,
                          float fontSize,
                          RaylibColor color,
                          Rectangle rect) {
        const Vector2 size = MeasureTextEx(font, text.c_str(), fontSize, 0.f);
        DrawTextEx(font,
                   text.c_str(),
                   Vector2{
                       rect.x + rect.width * 0.5f - size.x * 0.5f,
                       rect.y + rect.height * 0.5f - size.y * 0.5f,
                   },
                   fontSize,
                   0.f,
                   color);
    }
}
#endif

void GUIView::showBoard(const GameStateView& state) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();

    const float W = static_cast<float>(GetScreenWidth());
    const float H = static_cast<float>(GetScreenHeight());
    const float globeSize = H;
    const float globeX = (W - globeSize) * 0.5f;
    const float globeY = 0.f;
    const float panelW = (W - globeSize) * 0.5f;
    const float boardSize = H;
    const Rectangle boardBounds{(W - boardSize) * 0.5f, 0.f, boardSize, boardSize};

    ClearBackground(BG_COLOR);

    if (const Texture2D* globeTex = am.texture("assets/bg/GlobeWShadow.png")) {
        gui::draw::drawSprite(globeTex, Rectangle{globeX, globeY, globeSize, globeSize});
    }

    if (const Texture2D* titleTex = am.texture("assets/bg/Title.png")) {
        const float titleW = W * 0.28f;
        const float titleH = static_cast<float>(titleTex->height) * (titleW / static_cast<float>(titleTex->width));
        gui::draw::drawSprite(titleTex, Rectangle{
            (W - titleW) * 0.5f,
            (H - titleH) * 0.5f - 20.f,
            titleW,
            titleH,
        });
    }

    const int totalTiles = static_cast<int>(state.tiles.size());
    for (int i = 0; i < totalTiles; ++i) {
        const Rectangle tileBounds = gui::tile::boardTileBounds(i, boardBounds, totalTiles);
        gui::tile::drawTile(state.tiles[static_cast<size_t>(i)],
                            tileBounds,
                            gui::tile::boardSideForIndex(i, totalTiles));
    }

    const float tokenSpacing = boardSize * 0.025f;
    std::vector<std::vector<int>> playersAt(static_cast<size_t>(std::max(0, totalTiles)));
    for (int p = 0; p < static_cast<int>(state.players.size()); ++p) {
        const auto& player = state.players[static_cast<size_t>(p)];
        if (player.status != PlayerStatus::BANKRUPT) {
            const int pos = player.position;
            if (pos >= 0 && pos < totalTiles) {
                playersAt[static_cast<size_t>(pos)].push_back(p);
            }
        }
    }

    for (int i = 0; i < totalTiles; ++i) {
        const auto& stackedPlayers = playersAt[static_cast<size_t>(i)];
        if (stackedPlayers.empty()) {
            continue;
        }

        const Rectangle tileBounds = gui::tile::boardTileBounds(i, boardBounds, totalTiles);
        const float cx = tileBounds.x + tileBounds.width * 0.5f;
        const float cy = tileBounds.y + tileBounds.height * 0.5f;
        const float tokenBox = std::min(tileBounds.width, tileBounds.height) * 0.44f;
        const float tokenR = tokenBox * 0.32f;
        std::vector<Vector2> offsets;
        if (stackedPlayers.size() == 1) {
            offsets.push_back(Vector2{0.f, 0.f});
        } else if (stackedPlayers.size() == 2) {
            offsets.push_back(Vector2{-tokenSpacing * 0.5f, 0.f});
            offsets.push_back(Vector2{tokenSpacing * 0.5f, 0.f});
        } else if (stackedPlayers.size() == 3) {
            offsets.push_back(Vector2{-tokenSpacing * 0.5f, -tokenSpacing * 0.38f});
            offsets.push_back(Vector2{tokenSpacing * 0.5f, -tokenSpacing * 0.38f});
            offsets.push_back(Vector2{0.f, tokenSpacing * 0.48f});
        } else {
            offsets.push_back(Vector2{-tokenSpacing * 0.5f, -tokenSpacing * 0.5f});
            offsets.push_back(Vector2{tokenSpacing * 0.5f, -tokenSpacing * 0.5f});
            offsets.push_back(Vector2{-tokenSpacing * 0.5f, tokenSpacing * 0.5f});
            offsets.push_back(Vector2{tokenSpacing * 0.5f, tokenSpacing * 0.5f});
        }

        for (size_t stackIndex = 0; stackIndex < stackedPlayers.size(); ++stackIndex) {
            const int playerIdx = stackedPlayers[stackIndex];
            const auto& player = state.players[static_cast<size_t>(playerIdx)];
            const Vector2 offset = offsets[std::min(stackIndex, offsets.size() - 1)];
            const Texture2D* tex = am.texture(tokenTexturePath(setup_, player.username, playerIdx));
            if (tex) {
                const float scale = std::min(tokenBox / static_cast<float>(tex->width),
                                             tokenBox / static_cast<float>(tex->height));
                const float drawW = static_cast<float>(tex->width) * scale;
                const float drawH = static_cast<float>(tex->height) * scale;
                gui::draw::drawSprite(tex, Rectangle{
                    cx + offset.x - drawW * 0.5f,
                    cy + offset.y - drawH * 0.5f,
                    drawW,
                    drawH,
                });
            } else {
                const int colorIndex = selectedPlayerColor(setup_, player.username, playerIdx);
                DrawCircleV(Vector2{cx + offset.x, cy + offset.y},
                            tokenR,
                            gui::menu::setupPalette()[static_cast<size_t>(colorIndex)]);
                DrawCircleLines(static_cast<int>(cx + offset.x),
                                static_cast<int>(cy + offset.y),
                                tokenR,
                                gui::menu::makeColor(255, 255, 255, 200));
            }
        }
    }

    drawLeftPanel(state, panelW, H);
    drawRightPanel(state, panelW, H);

    if (diceAnimating_) {
        drawDiceAnimation(GetFrameTime());
    }

    if (saveLoadStatusFrames_ > 0) {
        --saveLoadStatusFrames_;
        const float toastW = W * 0.40f;
        const float toastH = H * 0.06f;
        const Rectangle toast = gui::draw::centeredRect(W * 0.5f, H * 0.04f + toastH * 0.5f, toastW, toastH);
        gui::draw::drawPanel(toast, gui::menu::makeColor(30, 50, 80, 220), gui::menu::makeColor(80, 130, 200));
        drawTextCentered(am.font("regular"), saveLoadStatus_, toastH * 0.40f, gui::menu::makeColor(230, 240, 255), toast);
    }

    if (currentPrompt_ && currentPrompt_->resolved) {
        buyPromptActive_ = false;
    }

    if (currentPrompt_ && currentPrompt_->type != GUIPromptType::NONE && !currentPrompt_->resolved) {
        renderPromptOverlay(*currentPrompt_);
    }
#else
    (void)state;
#endif
}

void GUIView::drawLeftPanel(const GameStateView& state, float panelW, float H) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();
    const float pad = 14.f;

    gui::draw::drawPanel(Rectangle{0.f, 0.f, panelW, H}, PANEL_BG, PANEL_BORDER);
    DrawRectangleRec(Rectangle{panelW - 1.f, 0.f, 2.f, H}, PANEL_BORDER);

    float y = pad;
    {
        const std::string title = "NIMONSPOLY";
        const float fontSize = panelW * 0.13f;
        const Font& font = am.font("extrabold");
        const Vector2 size = MeasureTextEx(font, title.c_str(), fontSize, 0.f);
        DrawTextEx(font,
                   title.c_str(),
                   Vector2{panelW * 0.5f - size.x * 0.5f, y},
                   fontSize,
                   0.f,
                   ACCENT_CYAN);
        y += fontSize + 6.f;
    }

    {
        const std::string roundText =
            "Round  " + std::to_string(state.currentTurn) + " / " + std::to_string(state.maxTurn);
        const float fontSize = panelW * 0.07f;
        const Font& font = am.font("regular");
        const Vector2 size = MeasureTextEx(font, roundText.c_str(), fontSize, 0.f);
        DrawTextEx(font,
                   roundText.c_str(),
                   Vector2{panelW * 0.5f - size.x * 0.5f, y},
                   fontSize,
                   0.f,
                   TEXT_MUTED);
        y += fontSize + 18.f;
    }

    DrawLineEx(Vector2{pad, y}, Vector2{panelW - pad, y}, 1.f, gui::menu::makeColor(255, 255, 255, 25));
    y += 16.f;

    const float rowH = 42.f;
    for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
        const auto& player = state.players[static_cast<size_t>(i)];
        const bool active = (player.username == state.currentPlayerName);
        const bool bankrupt = (player.status == PlayerStatus::BANKRUPT);
        const RaylibColor playerColor =
            gui::menu::setupPalette()[static_cast<size_t>(selectedPlayerColor(setup_, player.username, i))];

        if (active) {
            const Rectangle highlight{pad * 0.25f, y, panelW - pad * 0.5f, rowH};
            gui::draw::drawPanel(highlight,
                                 gui::menu::makeColor(playerColor.r, playerColor.g, playerColor.b, 35),
                                 gui::menu::makeColor(playerColor.r, playerColor.g, playerColor.b, 90));
        }

        DrawCircleV(Vector2{pad + 10.f, y + rowH * 0.5f},
                    6.f,
                    bankrupt ? gui::menu::makeColor(100, 100, 110) : playerColor);

        std::string displayName = player.username;
        if (displayName.size() > 12) {
            displayName = displayName.substr(0, 10) + "..";
        }
        DrawTextEx(am.font("bold"),
                   displayName.c_str(),
                   Vector2{pad + 24.f, y + 4.f},
                   panelW * 0.065f,
                   0.f,
                   bankrupt ? gui::menu::makeColor(100, 105, 115) : (active ? TEXT_PRIMARY : TEXT_MUTED));

        const std::string moneyText = "M" + std::to_string(player.money.getAmount());
        DrawTextEx(am.font("regular"),
                   moneyText.c_str(),
                   Vector2{pad + 24.f, y + rowH * 0.52f},
                   panelW * 0.055f,
                   0.f,
                   bankrupt ? gui::menu::makeColor(100, 105, 115) : TEXT_GREEN);

        if (bankrupt) {
            DrawTextEx(am.font("bold"),
                       "BANKRUPT",
                       Vector2{panelW - pad - 90.f, y + rowH * 0.30f},
                       panelW * 0.05f,
                       0.f,
                       gui::menu::makeColor(0xff, 0x4d, 0x4d));
        }

        y += rowH + 4.f;
    }

    y += 10.f;
    DrawLineEx(Vector2{pad, y}, Vector2{panelW - pad, y}, 1.f, gui::menu::makeColor(255, 255, 255, 25));
    y += 16.f;

    DrawTextEx(am.font("bold"),
               "GAME LOG",
               Vector2{panelW * 0.5f - MeasureTextEx(am.font("bold"), "GAME LOG", panelW * 0.07f, 0.f).x * 0.5f, y},
               panelW * 0.07f,
               0.f,
               TEXT_PRIMARY);
    y += panelW * 0.07f + 10.f;

    const int total = static_cast<int>(log_.size());
    const float lineH = 18.f;
    const float maxLogH = H - y - pad;
    float usedH = 0.f;
    struct VisibleLog {
        int idx;
        std::vector<std::string> lines;
    };
    std::vector<VisibleLog> visible;

    for (int li = total - 1; li >= 0; --li) {
        const auto& entry = log_[static_cast<size_t>(li)];
        const std::string full = "[" + entry.username + "] " + entry.detail;
        auto wrapped = gui::draw::wrapText(am, "regular", full, panelW * 0.055f, panelW - pad * 2.f);
        const float h = static_cast<float>(wrapped.size()) * lineH;
        if (usedH + h > maxLogH) {
            break;
        }
        visible.push_back({li, std::move(wrapped)});
        usedH += h;
    }

    float drawY = y;
    for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
        const RaylibColor color = (it->idx == total - 1) ? TEXT_PRIMARY : TEXT_MUTED;
        for (const auto& line : it->lines) {
            DrawTextEx(am.font("regular"), line.c_str(), Vector2{pad, drawY}, panelW * 0.055f, 0.f, color);
            drawY += lineH;
        }
    }

    if (log_.empty()) {
        DrawTextEx(am.font("regular"),
                   "No events yet",
                   Vector2{pad, drawY},
                   panelW * 0.055f,
                   0.f,
                   TEXT_MUTED);
    }
#else
    (void)state;
    (void)panelW;
    (void)H;
#endif
}

void GUIView::drawRightPanel(const GameStateView& state, float panelW, float H) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();
    const float W = static_cast<float>(GetScreenWidth());
    const float px = W - panelW;
    const float pad = 14.f;

    gui::draw::drawPanel(Rectangle{px, 0.f, panelW, H}, PANEL_BG, PANEL_BORDER);
    DrawRectangleRec(Rectangle{px, 0.f, 2.f, H}, PANEL_BORDER);

    float y = pad;
    const std::array<const char*, 6> actionLabels{"TEBUS", "BANGUN", "GADAI", "KARTU", "SIMPAN", "SELESAI"};
    const float btnW = (panelW - pad * 2.f - 8.f) * 0.5f;
    const float btnH = 40.f;
    const float gapX = 8.f;
    const float gapY = 8.f;

    for (int i = 0; i < 6; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const Rectangle rect{
            px + pad + col * (btnW + gapX),
            y + row * (btnH + gapY),
            btnW,
            btnH,
        };
        gui::draw::drawPanel(rect, BTN_BG, BTN_BORDER);
        drawTextCentered(am.font("bold"), actionLabels[static_cast<size_t>(i)], btnH * 0.38f, TEXT_PRIMARY, rect);
    }
    y += 3.f * (btnH + gapY) + 18.f;

    DrawLineEx(Vector2{px + pad, y}, Vector2{px + panelW - pad, y}, 1.f, gui::menu::makeColor(255, 255, 255, 25));
    y += 18.f;

    const bool diceDisabled = state.hasRolledDice;
    const Rectangle diceRect{px + pad, y, panelW - pad * 2.f, 56.f};
    gui::draw::drawPanel(diceRect,
                         diceDisabled ? gui::menu::makeColor(0x12, 0x1a, 0x2e)
                                      : gui::menu::makeColor(0x1a, 0x28, 0x45),
                         diceDisabled ? BTN_BORDER : ACCENT_GOLD);
    drawTextCentered(am.font("title"),
                     "LEMPAR DADU",
                     diceRect.height * 0.42f,
                     diceDisabled ? TEXT_MUTED : TEXT_PRIMARY,
                     diceRect);
    y += diceRect.height + 18.f;

    DrawLineEx(Vector2{px + pad, y}, Vector2{px + panelW - pad, y}, 1.f, gui::menu::makeColor(255, 255, 255, 25));
    y += 18.f;

    DrawTextEx(am.font("bold"),
               "YOUR PROPERTIES",
               Vector2{
                   px + panelW * 0.5f - MeasureTextEx(am.font("bold"), "YOUR PROPERTIES", panelW * 0.07f, 0.f).x * 0.5f,
                   y,
               },
               panelW * 0.07f,
               0.f,
               TEXT_PRIMARY);
    y += panelW * 0.07f + 12.f;

    std::vector<const PropertyView*> myProperties;
    for (const auto& property : state.properties) {
        if (property.ownerName == state.currentPlayerName) {
            myProperties.push_back(&property);
        }
    }

    if (myProperties.empty()) {
        DrawTextEx(am.font("regular"),
                   "No properties owned",
                   Vector2{px + pad, y},
                   panelW * 0.06f,
                   0.f,
                   TEXT_MUTED);
        return;
    }

    const float cardW = (panelW - pad * 2.f - 8.f) / 3.f;
    const float cardH = cardW * 1.35f;
    const float cardGap = 4.f;

    for (size_t idx = 0; idx < myProperties.size() && idx < 6; ++idx) {
        const auto* property = myProperties[idx];
        const int col = static_cast<int>(idx % 3);
        const int row = static_cast<int>(idx / 3);
        const Rectangle card{
            px + pad + col * (cardW + cardGap),
            y + row * (cardH + cardGap),
            cardW,
            cardH,
        };
        gui::draw::drawPanel(card, gui::menu::makeColor(0x0d, 0x14, 0x24), BTN_BORDER);

        ::Color group = ::Color::DEFAULT;
        for (const auto& tile : state.tiles) {
            if (tile.code == property->code) {
                group = tile.color;
                break;
            }
        }

        const RaylibColor stripColor = gui::draw::tileColor(group);
        if (group != ::Color::DEFAULT) {
            DrawRectangleRec(Rectangle{card.x, card.y, card.width, card.height * 0.22f}, stripColor);
        }

        RaylibColor codeColor = (group != ::Color::DEFAULT) ? stripColor : TEXT_MUTED;
        if (group == ::Color::YELLOW) {
            codeColor = gui::menu::makeColor(0x90, 0x87, 0x00);
        }
        drawTextCentered(am.font("bold"),
                         property->code,
                         card.height * 0.22f,
                         codeColor,
                         Rectangle{card.x, card.y + card.height * 0.38f, card.width, card.height * 0.40f});

        if (property->buildingLevel > 0) {
            const float dotR = 3.5f;
            const float dotY = card.y + card.height * 0.82f;
            const int level = std::min(property->buildingLevel, 5);
            const float startDX = card.x + card.width * 0.5f - (level - 1) * dotR * 1.6f;
            for (int d = 0; d < level; ++d) {
                DrawCircleV(Vector2{startDX + d * dotR * 3.2f, dotY},
                            dotR,
                            property->status == PropertyStatus::MORTGAGED
                                ? gui::menu::makeColor(150, 150, 150)
                                : TEXT_GREEN);
            }
        }

        if (property->status == PropertyStatus::MORTGAGED) {
            DrawRectangleRec(card, gui::menu::makeColor(0, 0, 0, 120));
            drawTextCentered(am.font("bold"),
                             "MRTG",
                             card.height * 0.16f,
                             gui::menu::makeColor(0xff, 0x4d, 0x4d),
                             card);
        }
    }
#else
    (void)state;
    (void)panelW;
    (void)H;
#endif
}

void GUIView::drawDieFace(float cx, float cy, float size, int face, unsigned fillRGB, unsigned dotRGB) {
#if NIMONSPOLY_ENABLE_RAYLIB
    const auto toColor = [](unsigned rgb) -> RaylibColor {
        return RaylibColor{
            static_cast<unsigned char>((rgb >> 16) & 0xFF),
            static_cast<unsigned char>((rgb >> 8) & 0xFF),
            static_cast<unsigned char>(rgb & 0xFF),
            255,
        };
    };

    const float half = size * 0.5f;
    const float radius = size * 0.08f;
    const float offset = size * 0.28f;
    const RaylibColor fill = toColor(fillRGB);
    const RaylibColor dot = toColor(dotRGB);
    const Rectangle bg{cx - half, cy - half, size, size};

    DrawRectangleRounded(bg, 0.12f, 8, fill);
    DrawRectangleLinesEx(bg, 2.f, gui::menu::makeColor(80, 90, 110));

    auto drawDot = [&](float dx, float dy) {
        DrawCircleV(Vector2{cx + dx, cy + dy}, radius, dot);
    };

    switch (face) {
        case 1:
            drawDot(0.f, 0.f);
            break;
        case 2:
            drawDot(-offset, -offset);
            drawDot(offset, offset);
            break;
        case 3:
            drawDot(-offset, -offset);
            drawDot(0.f, 0.f);
            drawDot(offset, offset);
            break;
        case 4:
            drawDot(-offset, -offset);
            drawDot(offset, -offset);
            drawDot(-offset, offset);
            drawDot(offset, offset);
            break;
        case 5:
            drawDot(-offset, -offset);
            drawDot(offset, -offset);
            drawDot(0.f, 0.f);
            drawDot(-offset, offset);
            drawDot(offset, offset);
            break;
        case 6:
            drawDot(-offset, -offset);
            drawDot(offset, -offset);
            drawDot(-offset, 0.f);
            drawDot(offset, 0.f);
            drawDot(-offset, offset);
            drawDot(offset, offset);
            break;
        default:
            break;
    }
#else
    (void)cx;
    (void)cy;
    (void)size;
    (void)face;
    (void)fillRGB;
    (void)dotRGB;
#endif
}

void GUIView::drawDiceAnimation(float dt) {
#if NIMONSPOLY_ENABLE_RAYLIB
    diceAnimElapsed_ += dt;
    if (diceAnimElapsed_ >= DICE_ANIM_DURATION) {
        diceAnimating_ = false;
        diceAnimElapsed_ = 0.f;
        return;
    }

    const float W = static_cast<float>(GetScreenWidth());
    const float H = static_cast<float>(GetScreenHeight());
    const float dieSz = 90.f;
    const float gap = 30.f;
    const float totalW = dieSz * 2.f + gap;

    DrawRectangleRec(Rectangle{0.f, 0.f, W, H}, gui::menu::makeColor(0, 0, 0, 80));

    static std::mt19937 rng(static_cast<unsigned>(std::random_device{}()));
    std::uniform_int_distribution<int> dist(1, 6);
    if (static_cast<int>(diceAnimElapsed_ * 10.f) % 2 == 0) {
        diceAnimFace1_ = dist(rng);
        diceAnimFace2_ = dist(rng);
    }

    drawDieFace(W * 0.5f - totalW * 0.5f + dieSz * 0.5f, H * 0.5f, dieSz, diceAnimFace1_, 0xFAFAF8, 0x282832);
    drawDieFace(W * 0.5f + totalW * 0.5f - dieSz * 0.5f, H * 0.5f, dieSz, diceAnimFace2_, 0xFAFAF8, 0x282832);
#else
    (void)dt;
#endif
}

void GUIView::renderPromptOverlay(const GUIPromptState& prompt) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();
    const float W = static_cast<float>(GetScreenWidth());
    const float H = static_cast<float>(GetScreenHeight());
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;

    DrawRectangleRec(Rectangle{0.f, 0.f, W, H}, gui::menu::makeColor(0, 0, 0, 160));

    float panelYOffset = 0.f;
    if (buyPromptActive_ && prompt.type == GUIPromptType::YES_NO) {
        const float cardW = W * 0.30f;
        const float cardH = H * 0.22f;
        const float cardY = cy - H * 0.22f;
        panelYOffset = cardH * 0.5f + 20.f;

        const Rectangle card{cx - cardW * 0.5f, cardY - cardH * 0.5f, cardW, cardH};
        gui::draw::drawPanel(card, gui::menu::makeColor(250, 250, 248), gui::menu::makeColor(60, 90, 140));
        drawTextCentered(am.font("bold"), buyPromptInfo_.code, cardH * 0.13f,
                         gui::menu::makeColor(35, 55, 90),
                         Rectangle{card.x, card.y + cardH * 0.04f, card.width, cardH * 0.18f});
        drawTextCentered(am.font("regular"), buyPromptInfo_.name, cardH * 0.10f,
                         gui::menu::makeColor(80, 100, 130),
                         Rectangle{card.x, card.y + cardH * 0.20f, card.width, cardH * 0.16f});
        drawTextCentered(am.font("regular"), "Harga: " + buyPromptInfo_.purchasePrice.toString(), cardH * 0.10f,
                         gui::menu::makeColor(20, 110, 70),
                         Rectangle{card.x, card.y + cardH * 0.42f, card.width, cardH * 0.14f});
        drawTextCentered(am.font("regular"), "Uangmu: " + buyPromptMoney_.toString(), cardH * 0.10f,
                         gui::menu::makeColor(100, 100, 120),
                         Rectangle{card.x, card.y + cardH * 0.60f, card.width, cardH * 0.14f});
    }

    const float panelW = W * 0.42f;
    const float panelH = H * 0.28f;
    const Rectangle panel{cx - panelW * 0.5f, cy - panelH * 0.5f + panelYOffset, panelW, panelH};
    gui::draw::drawPanel(panel, gui::menu::makeColor(250, 250, 252), gui::menu::makeColor(80, 130, 200));

    const float titleSize = panelH * 0.13f;
    const float bodySize = panelH * 0.10f;
    const float hintSize = panelH * 0.07f;

    drawTextCentered(am.font("bold"),
                     prompt.label,
                     titleSize,
                     gui::menu::makeColor(35, 55, 90),
                     Rectangle{panel.x, panel.y + panelH * 0.06f, panel.width, titleSize * 1.2f});

    float contentY = cy - panelH * 0.08f + panelYOffset;
    if (prompt.type == GUIPromptType::MENU_CHOICE) {
        float optionY = contentY;
        for (int i = 0; i < static_cast<int>(prompt.options.size()); ++i) {
            const std::string line = std::to_string(i) + ". " + prompt.options[static_cast<size_t>(i)];
            drawTextCentered(am.font("regular"),
                             line,
                             bodySize,
                             gui::menu::makeColor(60, 80, 120),
                             Rectangle{panel.x, optionY, panel.width, bodySize * 1.2f});
            optionY += bodySize * 1.4f;
        }
    } else if (prompt.type == GUIPromptType::YES_NO) {
        drawTextCentered(am.font("regular"),
                         "Ketik Y / N lalu Enter",
                         hintSize,
                         gui::menu::makeColor(120, 140, 170),
                         Rectangle{panel.x, contentY, panel.width, hintSize * 1.2f});
    } else if (prompt.type == GUIPromptType::AUCTION) {
        drawTextCentered(am.font("regular"),
                         "Ketik PASS atau BID <jumlah>",
                         hintSize,
                         gui::menu::makeColor(120, 140, 170),
                         Rectangle{panel.x, contentY, panel.width, hintSize * 1.2f});
    } else {
        drawTextCentered(am.font("regular"),
                         "Ketik jawaban lalu Enter",
                         hintSize,
                         gui::menu::makeColor(120, 140, 170),
                         Rectangle{panel.x, contentY, panel.width, hintSize * 1.2f});
    }

    const std::string buffer = "> " + prompt.textBuffer + "_";
    drawTextCentered(am.font("regular"),
                     buffer,
                     bodySize,
                     gui::menu::makeColor(40, 60, 100),
                     Rectangle{panel.x, cy + panelH * 0.14f + panelYOffset, panel.width, bodySize * 1.5f});

    if (prompt.resolved) {
        buyPromptActive_ = false;
    }
#else
    (void)prompt;
#endif
}

void GUIView::handleInGameClick(float mx, float my, std::string& outCommand, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_RAYLIB
    const float W = static_cast<float>(GetScreenWidth());
    const float H = static_cast<float>(GetScreenHeight());
    const float globeSize = H;
    const float panelW = (W - globeSize) * 0.5f;
    const float px = W - panelW;
    const float pad = 14.f;

    float y = pad;
    const float btnW = (panelW - pad * 2.f - 8.f) * 0.5f;
    const float btnH = 40.f;
    const float gapX = 8.f;
    const float gapY = 8.f;
    const std::array<const char*, 6> actions{"TEBUS", "BANGUN", "GADAI", "KARTU", "SIMPAN", "SELESAI"};

    for (int i = 0; i < 6; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const Rectangle rect{
            px + pad + col * (btnW + gapX),
            y + row * (btnH + gapY),
            btnW,
            btnH,
        };
        if (CheckCollisionPointRec(Vector2{mx, my}, rect)) {
            outCommand = actions[static_cast<size_t>(i)];
            return;
        }
    }
    y += 3.f * (btnH + gapY) + 18.f + 18.f;

    const Rectangle diceRect{px + pad, y, panelW - pad * 2.f, 56.f};
    if (CheckCollisionPointRec(Vector2{mx, my}, diceRect)) {
        if (!state.hasRolledDice && !diceAnimating_) {
            diceAnimating_ = true;
            diceAnimElapsed_ = 0.f;
            outCommand = "DADU";
        }
    }
#else
    (void)mx;
    (void)my;
    (void)outCommand;
    (void)state;
#endif
}
