#include "ui/GUIView.hpp"

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"
#include "utils/GameUtils.hpp"

#if NIMONSPOLY_ENABLE_RAYLIB
#include <algorithm>
#include <array>
#include <cstdio>
#include <ctime>
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
    const RaylibColor PANEL_BORDER = gui::menu::makeColor(0x5c, 0xd6, 0xff, 40);
    const RaylibColor ACCENT_CYAN = gui::menu::makeColor(0x5c, 0xd6, 0xff);
    const RaylibColor ACCENT_GOLD = gui::menu::makeColor(0xff, 0xc1, 0x07);
    const RaylibColor TEXT_PRIMARY = gui::menu::makeColor(0xe8, 0xed, 0xf2);
    const RaylibColor TEXT_MUTED = gui::menu::makeColor(0x7a, 0x85, 0x98);
    const RaylibColor TEXT_GREEN = gui::menu::makeColor(0x00, 0xd9, 0x8e);
    const RaylibColor BTN_BG = gui::menu::makeColor(0x12, 0x1a, 0x2e);
    const RaylibColor BTN_BORDER = gui::menu::makeColor(0x2a, 0x3f, 0x5c);
    const RaylibColor SURFACE_BG = gui::menu::makeColor(0x0c, 0x11, 0x19, 235);
    const RaylibColor SURFACE_ALT = gui::menu::makeColor(0x11, 0x18, 0x24, 245);

    struct InGameLayout {
        Rectangle summaryPanel;
        Rectangle leftInputPanel;
        Rectangle playersPanel;
        Rectangle boardPanel;
        Rectangle boardBounds;
        Rectangle logPanel;
        Rectangle propertiesPanel;
        Rectangle actionsPanel;
    };

    struct ActionLayout {
        Rectangle diceRect;
        Rectangle setDiceRect;
        std::array<Rectangle, 6> buttons;
        float tipY{0.f};
    };

    constexpr std::array<const char*, 6> ACTION_LABELS{
        "TEBUS", "BANGUN", "GADAI", "KARTU", "SIMPAN", "SELESAI"
    };

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
        return gui::menu::effectivePlayerColor(
            setup, findSetupPlayerIndex(setup, username, fallbackIndex));
    }

    int selectedPlayerCharacter(const SetupState& setup, const std::string& username, int fallbackIndex) {
        return gui::menu::selectedPlayerCharacter(
            setup, findSetupPlayerIndex(setup, username, fallbackIndex));
    }

    const PlayerView* activePlayerView(const GameStateView& state) {
        if (!state.currentPlayerName.empty()) {
            for (const PlayerView& player : state.players) {
                if (player.username == state.currentPlayerName) {
                    return &player;
                }
            }
        }

        if (state.activePlayerIndex >= 0 &&
            state.activePlayerIndex < static_cast<int>(state.players.size())) {
            return &state.players[static_cast<size_t>(state.activePlayerIndex)];
        }
        return nullptr;
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

    Rectangle insetRect(Rectangle rect, float padding) {
        return Rectangle{
            rect.x + padding,
            rect.y + padding,
            std::max(0.f, rect.width - padding * 2.f),
            std::max(0.f, rect.height - padding * 2.f),
        };
    }

    Rectangle panelBodyRect(Rectangle rect, float headerHeight = 54.f, float padding = 16.f) {
        return Rectangle{
            rect.x + padding,
            rect.y + headerHeight,
            std::max(0.f, rect.width - padding * 2.f),
            std::max(0.f, rect.height - headerHeight - padding),
        };
    }

    void drawPanelFrame(AssetManager& am,
                        Rectangle rect,
                        const std::string& title,
                        const std::string& subtitle,
                        RaylibColor fill = SURFACE_BG) {
        gui::draw::drawPanel(rect, fill, PANEL_BORDER);
        const Rectangle header{
            rect.x + 1.f,
            rect.y + 1.f,
            rect.width - 2.f,
            42.f,
        };
        DrawRectangleRec(header, gui::menu::makeColor(255, 255, 255, 10));
        DrawLineEx(Vector2{rect.x + 16.f, rect.y + 43.f},
                   Vector2{rect.x + rect.width - 16.f, rect.y + 43.f},
                   1.f,
                   gui::menu::makeColor(255, 255, 255, 18));

        DrawTextEx(am.font("bold"),
                   title.c_str(),
                   Vector2{rect.x + 16.f, rect.y + 10.f},
                   26.f,
                   0.f,
                   TEXT_PRIMARY);
        if (!subtitle.empty()) {
            const Vector2 subtitleSize = MeasureTextEx(am.font("regular"), subtitle.c_str(), 16.f, 0.f);
            DrawTextEx(am.font("regular"),
                       subtitle.c_str(),
                       Vector2{rect.x + rect.width - subtitleSize.x - 16.f, rect.y + 14.f},
                       16.f,
                       0.f,
                       TEXT_MUTED);
        }
    }

    InGameLayout makeInGameLayout(float screenW, float screenH) {
        const float margin = std::clamp(std::min(screenW, screenH) * 0.018f, 14.f, 24.f);
        const float gutter = margin;
        const float leftW = std::clamp(screenW * 0.19f, 250.f, 320.f);
        const float rightW = std::clamp(screenW * 0.23f, 300.f, 380.f);
        const float contentH = screenH - margin * 2.f;

        constexpr float kSummaryH = 88.f;
        constexpr float kPlayerRowH = 58.f;
        constexpr float kPanelHeaderH = 56.f;
        const float playersH = kPanelHeaderH + 4.f * kPlayerRowH + 3.f * 10.f + 14.f;
        const float inputH = std::max(80.f, contentH - kSummaryH - playersH - gutter * 2.f);

        const float sideTopH    = std::clamp(contentH * 0.55f, 320.f, contentH - 220.f - gutter);
        const float sideBottomH = contentH - sideTopH - gutter;
        const float centerTopH    = std::clamp(contentH * 0.70f, 380.f, contentH - 150.f - gutter);
        const float centerBottomH = contentH - centerTopH - gutter;

        InGameLayout layout{};
        layout.logPanel = {screenW - margin - rightW, margin, rightW, sideTopH};
        layout.actionsPanel = {layout.logPanel.x, margin + sideTopH + gutter, rightW, sideBottomH};

        const float centerX = margin + leftW + gutter;
        const float centerW = layout.logPanel.x - gutter - centerX;
        layout.boardPanel = {centerX, margin, centerW, centerTopH};
        layout.propertiesPanel = {centerX, margin + centerTopH + gutter, centerW, centerBottomH};

        layout.summaryPanel = {margin, margin, leftW, kSummaryH};
        layout.leftInputPanel = {margin, margin + kSummaryH + gutter, leftW, inputH};
        layout.playersPanel = {margin, margin + kSummaryH + gutter + inputH + gutter, leftW, playersH};

        const Rectangle boardArea = insetRect(layout.boardPanel, 10.f);
        const float boardSize = std::max(180.f, std::min(boardArea.width, boardArea.height));
        layout.boardBounds = gui::draw::centeredRect(
            boardArea.x + boardArea.width * 0.5f,
            boardArea.y + boardArea.height * 0.5f,
            boardSize, boardSize);

        return layout;
    }

    ActionLayout makeActionLayout(Rectangle panelRect) {
        const Rectangle body = insetRect(panelRect, 14.f);
        constexpr float kGap = 10.f;
        constexpr float kTipH = 20.f;
        constexpr float kTipGap = 6.f;

        const float diceH = std::clamp(body.height * 0.28f, 66.f, 96.f);
        const float setDiceH = std::clamp(body.height * 0.10f, 30.f, 40.f);
        const float gridTop = body.y + diceH + kGap + setDiceH + kGap;
        const float gridH = body.height - diceH - setDiceH - kGap * 2.f - kTipGap - kTipH;
        const float rowH = std::max(30.f, (gridH - kGap) / 2.f);
        const float colW = (body.width - kGap * 2.f) / 3.f;

        ActionLayout layout{};
        layout.diceRect = {body.x, body.y, body.width, diceH};
        layout.setDiceRect = {body.x, body.y + diceH + kGap, body.width, setDiceH};

        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 3; ++col) {
                const int i = row * 3 + col;
                layout.buttons[static_cast<size_t>(i)] = {
                    body.x + col * (colW + kGap),
                    gridTop + row * (rowH + kGap),
                    colW, rowH,
                };
            }
        }

        const float lastRowBottom = gridTop + 2.f * rowH + kGap;
        layout.tipY = lastRowBottom + kTipGap;

        return layout;
    }
}
#endif

void GUIView::showBoard(const GameStateView& state) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();

    const float W = static_cast<float>(GetScreenWidth());
    const float H = static_cast<float>(GetScreenHeight());
    const InGameLayout layout = makeInGameLayout(W, H);

    ClearBackground(BG_COLOR);
    DrawRectangleGradientV(0, 0, static_cast<int>(W), static_cast<int>(H),
                           gui::menu::makeColor(0x10, 0x14, 0x1e),
                           gui::menu::makeColor(0x06, 0x09, 0x12));
    DrawCircleGradient({W * 0.50f, H * 0.18f},
                       std::min(W, H) * 0.26f,
                       gui::menu::makeColor(0x5c, 0xd6, 0xff, 22),
                       RL_BLANK);
    DrawCircleGradient({W * 0.72f, H * 0.68f},
                       std::min(W, H) * 0.18f,
                       gui::menu::makeColor(0xff, 0xc1, 0x07, 12),
                       RL_BLANK);

    drawLeftPanel(state, layout.summaryPanel, layout.leftInputPanel, layout.playersPanel);
    drawRightPanel(state, layout.logPanel, layout.actionsPanel);
    drawPropertyPanel(state, layout.propertiesPanel);

    if (const Texture2D* globeTex = am.texture("assets/bg/GlobeWShadow.png")) {
        const float backSize = layout.boardBounds.width * 1.08f;
        gui::draw::drawSprite(globeTex,
                              gui::draw::centeredRect(
                                  layout.boardBounds.x + layout.boardBounds.width * 0.5f,
                                  layout.boardBounds.y + layout.boardBounds.height * 0.5f,
                                  backSize, backSize),
                              gui::menu::makeColor(255, 255, 255, 180));
    }
    DrawRectangleRounded(insetRect(layout.boardBounds, -8.f), 0.02f, 8, gui::menu::makeColor(0, 0, 0, 70));

    if (const Texture2D* logoTex = am.texture("assets/bg/Title.png")) {
        const float logoW = layout.boardBounds.width * 0.44f;
        const float logoH = logoW * static_cast<float>(logoTex->height) / static_cast<float>(logoTex->width);
        gui::draw::drawSprite(
            logoTex,
            gui::draw::centeredRect(layout.boardBounds.x + layout.boardBounds.width * 0.5f,
                                    layout.boardBounds.y + layout.boardBounds.height * 0.50f,
                                    logoW,
                                    logoH),
            gui::menu::makeColor(255, 255, 255, 165));
    }

    const int totalTiles = static_cast<int>(state.tiles.size());
    for (int i = 0; i < totalTiles; ++i) {
        const Rectangle tileBounds = gui::tile::boardTileBounds(i, layout.boardBounds, totalTiles);
        TileData tile = state.tiles[static_cast<size_t>(i)];
        if (tile.isOwnable && !tile.ownerName.empty()) {
            tile.ownerColorIndex = selectedPlayerColor(setup_, tile.ownerName, 0);
        }
        gui::tile::drawTile(tile,
                            tileBounds,
                            gui::tile::boardSideForIndex(i, totalTiles));
    }

    const float tokenSpacing = layout.boardBounds.width * 0.025f;
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

        const Rectangle tileBounds = gui::tile::boardTileBounds(i, layout.boardBounds, totalTiles);
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

    if (inspectedTileIndex_ >= 0 && inspectedTileIndex_ < static_cast<int>(state.tiles.size())) {
        const TileData& tile = state.tiles[static_cast<size_t>(inspectedTileIndex_)];

        DrawRectangleRec(Rectangle{0.f, 0.f, W, H}, gui::menu::makeColor(0, 0, 0, 180));

        const float panelW = std::min(W * 0.75f, 860.f);
        const float panelH = std::min(H * 0.62f, 540.f);
        const Rectangle panel = gui::draw::centeredRect(W * 0.5f, H * 0.5f, panelW, panelH);
        gui::draw::drawPanel(panel, gui::menu::makeColor(0x0e, 0x14, 0x1e, 245), gui::menu::makeColor(0x5c, 0xd6, 0xff, 90));

        const float padding = 22.f;
        const float tileW = panelH * 0.50f;
        const float tileH = panelH * 0.78f;
        const Rectangle tileRect{
            panel.x + padding,
            panel.y + (panelH - tileH) * 0.5f,
            tileW,
            tileH,
        };
        gui::tile::drawTile(tile, tileRect, gui::tile::BoardSide::BOTTOM);

        const float textX = tileRect.x + tileRect.width + padding + 10.f;
        const float textW = panel.x + panelW - textX - padding;
        const float textTop = panel.y + padding;
        const float textBottom = panel.y + panelH - padding - 24.f;
        const float textHeight = std::max(0.f, textBottom - textTop);

        struct TextLine {
            std::string text;
            float size;
            RaylibColor color;
            float indent;
        };
        std::vector<TextLine> lines;
        lines.reserve(24);

        lines.push_back({tile.name, 28.f, ACCENT_GOLD, 0.f});
        lines.push_back({"Kode: " + tile.code, 17.f, TEXT_MUTED, 0.f});

        std::string typeLabel;
        switch (tile.type) {
            case TileType::STREET: typeLabel = "Street"; break;
            case TileType::RAILROAD: typeLabel = "Railroad"; break;
            case TileType::UTILITY: typeLabel = "Utility"; break;
            case TileType::GO: typeLabel = "Start"; break;
            case TileType::JAIL: typeLabel = "Penjara"; break;
            case TileType::GO_TO_JAIL: typeLabel = "Masuk Penjara"; break;
            case TileType::CHANCE: typeLabel = "Kesempatan"; break;
            case TileType::COMMUNITY_CHEST: typeLabel = "Dana Umum"; break;
            case TileType::TAX_PPH: typeLabel = "Pajak PPH"; break;
            case TileType::TAX_PBM: typeLabel = "Pajak PBM"; break;
            case TileType::FESTIVAL: typeLabel = "Festival"; break;
            case TileType::FREE_PARKING: typeLabel = "Parkir Gratis"; break;
            default: typeLabel = "Lainnya"; break;
        }
        lines.push_back({"Jenis: " + typeLabel, 17.f, TEXT_PRIMARY, 0.f});

        if (tile.type == TileType::STREET) {
            lines.push_back({"Warna: " + colorName(tile.color), 17.f, TEXT_PRIMARY, 0.f});
        }

        if (tile.isOwnable) {
            lines.push_back({"Harga: M" + std::to_string(tile.price), 17.f, TEXT_GREEN, 0.f});
            lines.push_back({"Gadai: M" + std::to_string(tile.mortgageValue), 17.f, TEXT_PRIMARY, 0.f});
            if (!tile.ownerName.empty()) {
                lines.push_back({"Pemilik: " + tile.ownerName, 17.f, ACCENT_CYAN, 0.f});
            }
            if (tile.isMortgaged) {
                lines.push_back({"STATUS: GADAI", 17.f, gui::menu::makeColor(0xff, 0x4d, 0x4d), 0.f});
            }
        }

        if (tile.type == TileType::STREET && !tile.rentLevels.empty()) {
            lines.push_back({"", 8.f, TEXT_MUTED, 0.f});
            lines.push_back({"Tabel Sewa:", 17.f, TEXT_PRIMARY, 0.f});
            for (size_t r = 0; r < tile.rentLevels.size() && r < 6; ++r) {
                std::string levelLabel;
                if (r == 0) levelLabel = "Tanpa bangunan";
                else if (r == 5) levelLabel = "Hotel";
                else levelLabel = std::to_string(r) + " rumah";
                lines.push_back({levelLabel + ": M" + std::to_string(tile.rentLevels[r]), 16.f, TEXT_MUTED, 14.f});
            }
            lines.push_back({"", 4.f, TEXT_MUTED, 0.f});
            lines.push_back({"Rumah: M" + std::to_string(tile.houseCost), 16.f, TEXT_PRIMARY, 0.f});
            lines.push_back({"Hotel: M" + std::to_string(tile.hotelCost), 16.f, TEXT_PRIMARY, 0.f});
        }

        if (tile.type == TileType::RAILROAD && !tile.rentLevels.empty()) {
            lines.push_back({"", 8.f, TEXT_MUTED, 0.f});
            lines.push_back({"Tabel Sewa (jumlah stasiun dimiliki):", 17.f, TEXT_PRIMARY, 0.f});
            for (size_t r = 0; r < tile.rentLevels.size(); ++r) {
                lines.push_back({std::to_string(r + 1) + " stasiun: M" + std::to_string(tile.rentLevels[r]), 16.f, TEXT_MUTED, 14.f});
            }
        }

        if (tile.isOwnable && !tile.ownerName.empty() && !tile.isMortgaged) {
            lines.push_back({"", 8.f, TEXT_MUTED, 0.f});
            int currentRent = 0;
            if (tile.type == TileType::STREET && tile.buildingLevel >= 0 &&
                tile.buildingLevel < static_cast<int>(tile.rentLevels.size())) {
                currentRent = tile.rentLevels[tile.buildingLevel];
            } else if (tile.type == TileType::RAILROAD && !tile.rentLevels.empty()) {
                int railroadCount = 0;
                for (const auto& p : state.properties) {
                    if (p.ownerName == tile.ownerName && p.type == TileType::RAILROAD) {
                        ++railroadCount;
                    }
                }
                if (railroadCount > 0 && railroadCount <= static_cast<int>(tile.rentLevels.size())) {
                    currentRent = tile.rentLevels[railroadCount - 1];
                }
            }

            if (tile.festivalTurnsRemaining > 0 && tile.festivalMultiplier > 1) {
                currentRent *= tile.festivalMultiplier;
                lines.push_back({"Sewa saat ini (Festival x" + std::to_string(tile.festivalMultiplier) + "): M" + std::to_string(currentRent), 18.f, ACCENT_GOLD, 0.f});
            } else if (currentRent > 0) {
                lines.push_back({"Sewa saat ini: M" + std::to_string(currentRent), 18.f, ACCENT_GOLD, 0.f});
            } else if (tile.type == TileType::UTILITY) {
                lines.push_back({"Sewa saat ini: 4x dadu (1 utilitas) atau 10x dadu (2 utilitas)", 16.f, ACCENT_GOLD, 0.f});
            }
        }

        float totalContentH = 0.f;
        for (const auto& line : lines) {
            totalContentH += line.size + 6.f;
        }
        totalContentH += 8.f;

        const int maxScroll = static_cast<int>(std::max(0.f, totalContentH - textHeight));
        const Rectangle textRegion{textX, textTop, textW, textHeight};
        if (CheckCollisionPointRec(GetMousePosition(), textRegion)) {
            const float wheel = GetMouseWheelMove();
            if (wheel != 0.f) {
                tileOverlayScrollPx_ -= static_cast<int>(wheel * 36.f);
            }
        }
        tileOverlayScrollPx_ = std::clamp(tileOverlayScrollPx_, 0, maxScroll);

        BeginScissorMode(static_cast<int>(textRegion.x),
                         static_cast<int>(textRegion.y),
                         static_cast<int>(textRegion.width),
                         static_cast<int>(textRegion.height));
        float drawY = textTop - static_cast<float>(tileOverlayScrollPx_);
        for (const auto& line : lines) {
            if (line.text.empty()) {
                drawY += line.size;
                continue;
            }
            DrawTextEx(am.font(line.size >= 20.f ? "bold" : "regular"), line.text.c_str(),
                       Vector2{textX + line.indent, drawY}, line.size, 0.f, line.color);
            drawY += line.size + 6.f;
        }
        EndScissorMode();

        if (maxScroll > 0) {
            const float trackX = textX + textW - 5.f;
            DrawRectangleRounded(Rectangle{trackX, textTop, 3.f, textHeight},
                                 1.f, 2, gui::menu::makeColor(255, 255, 255, 24));
            const float thumbH = std::max(24.f, textHeight * (textHeight / totalContentH));
            const float travel = std::max(0.f, textHeight - thumbH);
            const float t = (maxScroll == 0) ? 0.f : static_cast<float>(tileOverlayScrollPx_) / static_cast<float>(maxScroll);
            DrawRectangleRounded(Rectangle{trackX, textTop + travel * t, 3.f, thumbH},
                                 1.f, 2, gui::menu::makeColor(0x7d, 0xb4, 0xec, 220));
        }

        DrawTextEx(am.font("regular"), "Klik luar panel untuk tutup",
                   Vector2{panel.x + panelW * 0.5f - 90.f, panel.y + panelH - 24.f}, 14.f, 0.f, TEXT_MUTED);
    }

#else
    (void)state;
#endif
}

#if NIMONSPOLY_ENABLE_RAYLIB
void GUIView::drawLeftPanel(const GameStateView& state, Rectangle summaryRect, Rectangle inputRect, Rectangle playersRect) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();

    gui::draw::drawPanel(summaryRect, SURFACE_BG, PANEL_BORDER);
    {
        const float midY = summaryRect.y + summaryRect.height * 0.5f;
        const std::string roundText = "Ronde " + std::to_string(state.currentTurn) + " / " + std::to_string(state.maxTurn);
        DrawTextEx(am.font("bold"), roundText.c_str(), Vector2{summaryRect.x + 14.f, midY - 14.f}, 19.f, 0.f, TEXT_PRIMARY);
        DrawTextEx(am.font("regular"), "putaran", Vector2{summaryRect.x + 14.f, midY + 7.f}, 13.f, 0.f, TEXT_MUTED);
        const std::string clockStr = formattedElapsedTime();
        const Vector2 clockMeasure = MeasureTextEx(am.font("bold"), clockStr.c_str(), 22.f, 0.f);
        DrawTextEx(am.font("bold"), clockStr.c_str(),
                   Vector2{summaryRect.x + summaryRect.width - clockMeasure.x - 14.f, midY - 14.f},
                   22.f, 0.f, ACCENT_CYAN);
        DrawTextEx(am.font("regular"), "durasi",
                   Vector2{summaryRect.x + summaryRect.width - clockMeasure.x - 14.f, midY + 7.f},
                   13.f, 0.f, TEXT_MUTED);
    }

    const bool promptActive =
        currentPrompt_ &&
        currentPrompt_->type != GUIPromptType::NONE &&
        !currentPrompt_->resolved;
    if (!promptActive) {
        promptOptionScrollPx_ = 0;
        promptOptionScrollType_ = GUIPromptType::NONE;
        promptOptionScrollCount_ = 0;
        promptOptionScrollLabel_.clear();
        promptOptionWrapped_.clear();
        promptOptionColors_.clear();
        promptOptionHeights_.clear();
        promptOptionTotalHeight_ = 0.f;
    }
    drawPanelFrame(am, inputRect, "Pop Up", promptActive ? "aktif" : "standby", SURFACE_BG);
    {
        const Rectangle body = panelBodyRect(inputRect, 56.f, 14.f);
        const Rectangle inputBox{
            body.x,
            body.y + body.height - 52.f,
            body.width,
            52.f,
        };
        const float contentBottom = inputBox.y - 12.f;
        float cy = body.y;

        if (!promptActive) {
            const Rectangle idleCard{body.x, cy, body.width, std::min(112.f, body.height * 0.34f)};
            gui::draw::drawPanel(idleCard,
                                 gui::menu::makeColor(0x12, 0x19, 0x25, 245),
                                 gui::menu::makeColor(0x39, 0x4e, 0x68));
            DrawTextEx(am.font("bold"),
                       "Belum ada pop-up aktif.",
                       Vector2{idleCard.x + 12.f, idleCard.y + 12.f},
                       18.f,
                       0.f,
                       TEXT_PRIMARY);
            auto idleLines = gui::draw::wrapText(
                am,
                "regular",
                "Saat engine meminta input, prompt akan muncul di panel ini tanpa menutup board.",
                14.f,
                idleCard.width - 24.f);
            float idleY = idleCard.y + 40.f;
            for (size_t i = 0; i < idleLines.size() && i < 3; ++i) {
                DrawTextEx(am.font("regular"),
                           idleLines[i].c_str(),
                           Vector2{idleCard.x + 12.f, idleY},
                           14.f,
                           0.f,
                           TEXT_MUTED);
                idleY += 17.f;
            }
            cy = idleCard.y + idleCard.height + 16.f;

            const std::string activeText = state.currentPlayerName.empty() ? "Menunggu giliran" : state.currentPlayerName;
            DrawTextEx(am.font("bold"), "Giliran aktif", Vector2{body.x, cy}, 14.f, 0.f, TEXT_MUTED);
            cy += 18.f;
            DrawTextEx(am.font("semibold"), activeText.c_str(), Vector2{body.x, cy}, 18.f, 0.f, TEXT_PRIMARY);
            cy += 32.f;

            const PlayerView* activePlayer = activePlayerView(state);
            std::string tip;
            if (state.extraRollAvailable) {
                tip = "Double aktif. Pemain ini wajib lempar dadu lagi sebelum bisa mengakhiri giliran.";
            } else if (activePlayer && activePlayer->status == PlayerStatus::JAILED) {
                tip = "Pemain aktif sedang di penjara. Lempar untuk mencoba keluar; hanya double yang membuat pemain bergerak.";
            } else if (state.hasRolledDice) {
                tip = "Bangun, gadai, tebus, gunakan kartu, atau akhiri giliran.";
            } else {
                tip = "Lempar dadu untuk memulai aksi utama pada giliran ini.";
            }
            auto tipLines = gui::draw::wrapText(am, "regular", tip, 14.f, body.width);
            for (size_t i = 0; i < tipLines.size() && cy < contentBottom - 16.f; ++i) {
                DrawTextEx(am.font("regular"),
                           tipLines[i].c_str(),
                           Vector2{body.x, cy},
                           14.f,
                           0.f,
                           TEXT_MUTED);
                cy += 17.f;
            }

            if (lastD1_ > 0 && state.hasRolledDice && cy + 54.f < contentBottom) {
                cy += 10.f;
                DrawTextEx(am.font("bold"), "Dadu terakhir", Vector2{body.x, cy}, 14.f, 0.f, TEXT_MUTED);
                cy += 20.f;
                constexpr float kDiceSz = 34.f;
                drawDieFace(body.x + kDiceSz * 0.5f, cy + kDiceSz * 0.5f, kDiceSz, lastD1_, 0xFAFAF8, 0x282832);
                drawDieFace(body.x + kDiceSz + 12.f + kDiceSz * 0.5f, cy + kDiceSz * 0.5f, kDiceSz, lastD2_, 0xFAFAF8, 0x282832);
                DrawTextEx(am.font("semibold"),
                           ("= " + std::to_string(lastD1_ + lastD2_)).c_str(),
                           Vector2{body.x + kDiceSz * 2.f + 20.f, cy + 4.f},
                           23.f,
                           0.f,
                           ACCENT_GOLD);
            }
        } else {
            const GUIPromptState& prompt = *currentPrompt_;

            if (buyPromptActive_ && prompt.type == GUIPromptType::YES_NO) {
                const Rectangle card{body.x, cy, body.width, std::min(112.f, contentBottom - body.y)};
                gui::draw::drawPanel(card,
                                     gui::menu::makeColor(0xf5, 0xf7, 0xfb, 250),
                                     gui::menu::makeColor(0x5c, 0x82, 0xb5));
                DrawTextEx(am.font("bold"),
                           buyPromptInfo_.code.c_str(),
                           Vector2{card.x + 12.f, card.y + 10.f},
                           24.f,
                           0.f,
                           gui::menu::makeColor(0x23, 0x3e, 0x64));
                DrawTextEx(am.font("semibold"),
                           buyPromptInfo_.name.c_str(),
                           Vector2{card.x + 12.f, card.y + 40.f},
                           18.f,
                           0.f,
                           gui::menu::makeColor(0x2f, 0x49, 0x6d));
                DrawTextEx(am.font("regular"),
                           ("Harga: " + buyPromptInfo_.purchasePrice.toString()).c_str(),
                           Vector2{card.x + 12.f, card.y + 68.f},
                           15.f,
                           0.f,
                           gui::menu::makeColor(0x15, 0x7a, 0x4a));
                DrawTextEx(am.font("regular"),
                           ("Uangmu: " + buyPromptMoney_.toString()).c_str(),
                           Vector2{card.x + 12.f, card.y + 88.f},
                           15.f,
                           0.f,
                           gui::menu::makeColor(0x6a, 0x75, 0x86));
                cy = card.y + card.height + 14.f;
            }

            DrawTextEx(am.font("bold"), "Prompt aktif", Vector2{body.x, cy}, 14.f, 0.f, TEXT_MUTED);
            cy += 22.f;
            auto promptLines = gui::draw::wrapText(am, "bold", prompt.label, 18.f, body.width);
            for (size_t i = 0; i < promptLines.size() && cy < contentBottom - 90.f; ++i) {
                DrawTextEx(am.font("bold"),
                           promptLines[i].c_str(),
                           Vector2{body.x, cy},
                           18.f,
                           0.f,
                           TEXT_PRIMARY);
                cy += 22.f;
            }
            cy += 6.f;

            std::string hint = "Ketik jawaban lalu Enter.";
            if (prompt.type == GUIPromptType::YES_NO) {
                hint = "Input: Y atau N";
            } else if (prompt.type == GUIPromptType::AUCTION) {
                hint = "Input: PASS atau BID <jumlah>";
            } else if (prompt.type == GUIPromptType::MENU_CHOICE ||
                       prompt.type == GUIPromptType::NUMBER ||
                       prompt.type == GUIPromptType::PLAYER_COUNT ||
                       prompt.type == GUIPromptType::TAX_CHOICE ||
                       prompt.type == GUIPromptType::LIQUIDATION ||
                       prompt.type == GUIPromptType::SKILL_CARD) {
                hint = "Input: angka 1-N sesuai pilihan";
            } else if (prompt.type == GUIPromptType::DICE_MANUAL) {
                hint = "Input: dua angka, misal 3 5. Esc untuk batal.";
            }

            auto hintLines = gui::draw::wrapText(am, "regular", hint, 14.f, body.width);
            for (size_t i = 0; i < hintLines.size() && cy < contentBottom - 60.f; ++i) {
                DrawTextEx(am.font("regular"),
                           hintLines[i].c_str(),
                           Vector2{body.x, cy},
                           14.f,
                           0.f,
                           TEXT_MUTED);
                cy += 17.f;
            }

              if ((prompt.type == GUIPromptType::MENU_CHOICE ||
                  prompt.type == GUIPromptType::LIQUIDATION ||
                 prompt.type == GUIPromptType::SKILL_CARD) &&
                !prompt.options.empty()) {
                const float listTop = cy + 6.f;
                const float listHeight = std::max(0.f, contentBottom - 18.f - listTop);
                if (listHeight > 2.f) {
                    const bool promptChanged =
                        promptOptionScrollType_ != prompt.type ||
                        promptOptionScrollCount_ != static_cast<int>(prompt.options.size()) ||
                        promptOptionScrollLabel_ != prompt.label;

                    if (promptChanged) {
                        promptOptionScrollPx_ = 0;
                        promptOptionScrollType_ = prompt.type;
                        promptOptionScrollCount_ = static_cast<int>(prompt.options.size());
                        promptOptionScrollLabel_ = prompt.label;

                        promptOptionWrapped_.clear();
                        promptOptionColors_.clear();
                        promptOptionHeights_.clear();
                        promptOptionTotalHeight_ = 0.f;
                        promptOptionWrapped_.reserve(prompt.options.size());
                        promptOptionColors_.reserve(prompt.options.size());
                        promptOptionHeights_.reserve(prompt.options.size());

                        for (int i = 0; i < static_cast<int>(prompt.options.size()); ++i) {
                            std::string optionLine;
                            RaylibColor optionColor = TEXT_PRIMARY;
                            if (prompt.type == GUIPromptType::SKILL_CARD) {
                                optionLine = prompt.options[static_cast<size_t>(i)];
                                optionColor = (i % 2 == 0) ? TEXT_PRIMARY : TEXT_MUTED;
                            } else {
                                optionLine = std::to_string(i + 1) + ". " + prompt.options[static_cast<size_t>(i)];
                            }

                            auto optionLines = gui::draw::wrapText(am, "regular", optionLine, 14.f, body.width - 14.f);
                            if (optionLines.empty()) {
                                optionLines.push_back(" ");
                            }

                            const float gapAfter =
                                (prompt.type == GUIPromptType::SKILL_CARD && i % 2 == 1) ? 8.f : 4.f;
                            const float entryHeight = static_cast<float>(optionLines.size()) * 16.f + gapAfter;
                            promptOptionWrapped_.push_back(std::move(optionLines));
                            promptOptionColors_.push_back(optionColor);
                            promptOptionHeights_.push_back(entryHeight);
                            promptOptionTotalHeight_ += entryHeight;
                        }
                    }

                    const int maxScroll =
                        std::max(0, static_cast<int>(std::ceil(promptOptionTotalHeight_ - listHeight)));

                    const Rectangle listRect{body.x, listTop, body.width, listHeight};
                    if (CheckCollisionPointRec(GetMousePosition(), listRect)) {
                        const float wheel = GetMouseWheelMove();
                        if (wheel != 0.f) {
                            promptOptionScrollPx_ -= static_cast<int>(wheel * 32.f);
                        }
                    }
                    if (IsKeyPressed(KEY_DOWN)) {
                        promptOptionScrollPx_ += 24;
                    }
                    if (IsKeyPressed(KEY_UP)) {
                        promptOptionScrollPx_ -= 24;
                    }
                    promptOptionScrollPx_ = std::clamp(promptOptionScrollPx_, 0, maxScroll);

                    BeginScissorMode(static_cast<int>(listRect.x),
                                     static_cast<int>(listRect.y),
                                     static_cast<int>(listRect.width),
                                     static_cast<int>(listRect.height));
                    float drawY = listTop - static_cast<float>(promptOptionScrollPx_);
                    for (size_t i = 0; i < promptOptionWrapped_.size(); ++i) {
                        const auto& lines = promptOptionWrapped_[i];
                        const RaylibColor color = promptOptionColors_[i];

                        for (const std::string& line : lines) {
                            DrawTextEx(am.font("regular"),
                                       line.c_str(),
                                       Vector2{body.x + 6.f, drawY},
                                       14.f,
                                       0.f,
                                       color);
                            drawY += 16.f;
                        }

                        drawY += promptOptionHeights_[i] - static_cast<float>(lines.size()) * 16.f;
                    }
                    EndScissorMode();

                    if (maxScroll > 0) {
                        const float trackX = body.x + body.width - 4.f;
                        DrawRectangleRounded(Rectangle{trackX, listTop, 3.f, listHeight},
                                             1.f,
                                             2,
                                             gui::menu::makeColor(255, 255, 255, 24));
                        const float thumbH = std::max(24.f, listHeight * (listHeight / promptOptionTotalHeight_));
                        const float travel = std::max(0.f, listHeight - thumbH);
                        const float t = (maxScroll == 0)
                                            ? 0.f
                                            : static_cast<float>(promptOptionScrollPx_) / static_cast<float>(maxScroll);
                        DrawRectangleRounded(Rectangle{trackX,
                                                       listTop + travel * t,
                                                       3.f,
                                                       thumbH},
                                             1.f,
                                             2,
                                             gui::menu::makeColor(0x7d, 0xb4, 0xec, 220));

                        DrawTextEx(am.font("regular"),
                                   "Scroll: wheel / Up-Down",
                                   Vector2{body.x, contentBottom - 14.f},
                                   12.f,
                                   0.f,
                                   TEXT_MUTED);
                    }
                }
            }
        }

        gui::draw::drawPanel(inputBox,
                             promptActive ? gui::menu::makeColor(0x16, 0x1d, 0x2c, 250)
                                         : gui::menu::makeColor(0x11, 0x17, 0x23, 240),
                             promptActive ? gui::menu::makeColor(0x6b, 0x8f, 0xc0)
                                         : gui::menu::makeColor(0x3f, 0x54, 0x71));
        const std::string buffer = promptActive
            ? ("> " + currentPrompt_->textBuffer + "_")
            : "> menunggu prompt";
        DrawTextEx(am.font("regular"),
                   buffer.c_str(),
                   Vector2{inputBox.x + 12.f, inputBox.y + inputBox.height * 0.5f - 10.f},
                   18.f,
                   0.f,
                   promptActive ? gui::menu::makeColor(220, 230, 246) : TEXT_MUTED);
    }

    drawPanelFrame(am, playersRect, "Daftar Pemain",
                   std::to_string(state.players.size()) + " pemain", SURFACE_BG);
    {
        const Rectangle body = panelBodyRect(playersRect, 56.f, 14.f);
        std::vector<int> displayOrder(state.players.size());
        for (int i = 0; i < static_cast<int>(displayOrder.size()); ++i) {
            displayOrder[static_cast<size_t>(i)] = i;
        }

        constexpr float rowGap = 10.f;
        constexpr float rowH = 62.f;
        float y = body.y;
        for (int orderIndex = 0; orderIndex < static_cast<int>(displayOrder.size()); ++orderIndex) {
            const int playerIndex = displayOrder[static_cast<size_t>(orderIndex)];
            const auto& player = state.players[static_cast<size_t>(playerIndex)];
            const bool active = player.username == state.currentPlayerName;
            const bool inspected = player.username == propertyPanelOwner_;
            const bool bankrupt = player.status == PlayerStatus::BANKRUPT;
            const RaylibColor playerColor =
                gui::menu::setupPalette()[static_cast<size_t>(selectedPlayerColor(setup_, player.username, playerIndex))];

            const Rectangle row{body.x, y, body.width, rowH};
            gui::draw::drawPanel(row,
                                 active ? gui::menu::makeColor(playerColor.r, playerColor.g, playerColor.b, 26)
                                        : gui::menu::makeColor(0x10, 0x17, 0x22, 240),
                                 active ? gui::menu::makeColor(playerColor.r, playerColor.g, playerColor.b, 120)
                                        : gui::menu::makeColor(0x33, 0x45, 0x5d));

            if (active) {
                DrawRectangleRec(
                    Rectangle{row.x, row.y, 5.f, row.height},
                    gui::menu::makeColor(playerColor.r, playerColor.g, playerColor.b, 220));
            }

            if (inspected) {
                DrawRectangleLinesEx(row, 2.f, gui::menu::makeColor(0xff, 0xc1, 0x07, 200));
                const std::string viewLabel = "Lihat";
                const Vector2 viewSize = MeasureTextEx(am.font("regular"), viewLabel.c_str(), 12.f, 0.f);
                DrawTextEx(am.font("regular"), viewLabel.c_str(),
                           Vector2{row.x + row.width - viewSize.x - 10.f, row.y + row.height - 18.f},
                           12.f, 0.f, gui::menu::makeColor(0xff, 0xc1, 0x07, 200));
            }

            const Vector2 markerCenter{row.x + 24.f, row.y + row.height * 0.5f};
            DrawCircleV(markerCenter,
                        12.f,
                        bankrupt ? gui::menu::makeColor(115, 120, 128) : playerColor);
            DrawCircleLinesV(markerCenter,
                             12.f,
                             active ? gui::menu::makeColor(255, 255, 255, 220)
                                    : gui::menu::makeColor(255, 255, 255, 70));

            std::string name = player.username;
            if (name.size() > 15) {
                name = name.substr(0, 13) + "..";
            }
            DrawTextEx(am.font("bold"), name.c_str(), Vector2{row.x + 46.f, row.y + 10.f}, 17.f, 0.f,
                       bankrupt ? gui::menu::makeColor(120, 125, 135) : TEXT_PRIMARY);

            const std::string meta = std::to_string(player.propertyCount) + " properti  |  " +
                                     std::to_string(player.skillCardCount) + " kartu";
            DrawTextEx(am.font("regular"), meta.c_str(),
                       Vector2{row.x + 46.f, row.y + 33.f}, 13.f, 0.f,
                       bankrupt ? gui::menu::makeColor(98, 104, 116) : TEXT_MUTED);

            const std::string moneyLabel = "M" + std::to_string(player.money.getAmount());
            const Vector2 moneySize = MeasureTextEx(am.font("bold"), moneyLabel.c_str(), 18.f, 0.f);
            DrawTextEx(am.font("bold"),
                       moneyLabel.c_str(),
                       Vector2{row.x + row.width - moneySize.x - 14.f, row.y + 12.f},
                       18.f,
                       0.f,
                       bankrupt ? gui::menu::makeColor(112, 118, 126) : TEXT_GREEN);

            y += rowH + rowGap;
        }
    }
#else
    (void)state;
    (void)summaryRect;
    (void)inputRect;
    (void)playersRect;
#endif
}

void GUIView::drawRightPanel(const GameStateView& state, Rectangle logRect, Rectangle actionsRect) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();

    drawPanelFrame(am,
                   logRect,
                   "Log",
                   std::to_string(log_.size()) + " entri",
                   SURFACE_BG);
    {
        const Rectangle body = panelBodyRect(logRect, 56.f, 14.f);
        const Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, logRect)) {
            const float wheel = GetMouseWheelMove();
            if (wheel > 0.f) {
                logScrollOffset_ = std::max(0, logScrollOffset_ - 1);
            } else if (wheel < 0.f) {
                logScrollOffset_ += 1;
            }
        }

        if (log_.empty()) {
            DrawTextEx(am.font("regular"),
                       "Belum ada log yang tercatat.",
                       Vector2{body.x, body.y + 6.f},
                       16.f,
                       0.f,
                       TEXT_MUTED);
        } else {
            logScrollOffset_ = std::clamp(logScrollOffset_, 0, std::max(0, static_cast<int>(log_.size()) - 1));
            float y = body.y;
            int drawn = 0;
            for (int offset = logScrollOffset_; offset < static_cast<int>(log_.size()); ++offset) {
                const auto& entry = log_[log_.size() - 1 - static_cast<size_t>(offset)];
                const std::string header = "T" + std::to_string(entry.turn) + "  " + entry.actionType;
                const std::string detail = entry.username.empty() ? entry.detail : ("[" + entry.username + "] " + entry.detail);
                auto wrapped = gui::draw::wrapText(am, "regular", detail, 15.f, body.width - 26.f);
                const float cardH = 32.f + static_cast<float>(wrapped.size()) * 17.f;
                if (y + cardH > body.y + body.height) {
                    break;
                }

                const Rectangle card{body.x, y, body.width - 8.f, cardH};
                gui::draw::drawPanel(card,
                                     gui::menu::makeColor(0x11, 0x18, 0x24, 250),
                                     gui::menu::makeColor(0x33, 0x45, 0x5d));
                DrawTextEx(am.font("bold"),
                           header.c_str(),
                           Vector2{card.x + 10.f, card.y + 8.f},
                           14.f,
                           0.f,
                           drawn == 0 ? TEXT_PRIMARY : TEXT_MUTED);

                float textY = card.y + 24.f;
                for (const std::string& line : wrapped) {
                    DrawTextEx(am.font("regular"),
                               line.c_str(),
                               Vector2{card.x + 10.f, textY},
                               15.f,
                               0.f,
                               TEXT_PRIMARY);
                    textY += 16.f;
                }

                y += cardH + 8.f;
                ++drawn;
            }
        }
    }

    gui::draw::drawPanel(actionsRect, SURFACE_BG, PANEL_BORDER);
    {
        const ActionLayout actionLayout = makeActionLayout(actionsRect);
        const PlayerView* activePlayer = activePlayerView(state);
        const bool activeJailed = activePlayer && activePlayer->status == PlayerStatus::JAILED;
        const bool diceDisabled = state.hasRolledDice;
        const std::string diceTitle =
            state.extraRollAvailable ? "LEMPAR LAGI" :
            (activeJailed ? "COBA KABUR" : "LEMPAR DADU");
        const std::string diceSubtitle =
            diceDisabled ? "Sudah dilempar" :
            (state.extraRollAvailable ? "Double: bonus roll aktif" :
             (activeJailed ? "Harus double untuk keluar" : "Aksi utama giliran ini"));
        gui::draw::drawPanel(actionLayout.diceRect,
                             diceDisabled ? gui::menu::makeColor(0x17, 0x1c, 0x27)
                                          : gui::menu::makeColor(0x26, 0x31, 0x47),
                             diceDisabled ? gui::menu::makeColor(0x4b, 0x57, 0x67)
                                          : ACCENT_GOLD);
        drawTextCentered(am.font("title"),
                         diceTitle,
                         26.f,
                         diceDisabled ? TEXT_MUTED : TEXT_PRIMARY,
                         Rectangle{
                             actionLayout.diceRect.x,
                             actionLayout.diceRect.y + 4.f,
                             actionLayout.diceRect.width,
                             actionLayout.diceRect.height * 0.56f,
                         });
        drawTextCentered(am.font("regular"),
                         diceSubtitle,
                         14.f,
                         diceDisabled ? TEXT_MUTED : gui::menu::makeColor(0xff, 0xe7, 0x9c),
                         Rectangle{
                             actionLayout.diceRect.x,
                             actionLayout.diceRect.y + actionLayout.diceRect.height * 0.60f,
                             actionLayout.diceRect.width,
                             actionLayout.diceRect.height * 0.30f,
                         });

        const bool setDiceDisabled = state.hasRolledDice || state.extraRollAvailable || activeJailed;
        gui::draw::drawPanel(actionLayout.setDiceRect,
                             setDiceDisabled ? gui::menu::makeColor(0x11, 0x16, 0x22)
                                             : gui::menu::makeColor(0x16, 0x22, 0x34),
                             setDiceDisabled ? gui::menu::makeColor(0x37, 0x43, 0x54)
                                             : gui::menu::makeColor(0x76, 0x9a, 0xd9));
        drawTextCentered(am.font("bold"),
                         "ATUR DADU",
                         16.f,
                         setDiceDisabled ? TEXT_MUTED : gui::menu::makeColor(0xdf, 0xe8, 0xff),
                         actionLayout.setDiceRect);

        for (size_t i = 0; i < ACTION_LABELS.size(); ++i) {
            const Rectangle rect = actionLayout.buttons[i];
            gui::draw::drawPanel(rect, BTN_BG, BTN_BORDER);
            drawTextCentered(am.font("bold"), ACTION_LABELS[i], 14.f, TEXT_PRIMARY, rect);
        }

        DrawTextEx(am.font("regular"),
                   activeJailed
                       ? "Saat di penjara, fokus pada percobaan keluar. Atur dadu dimatikan untuk state ini."
                       : "Bangun, gadai, tebus, simpan, atau buka kartu dari panel ini.",
                   Vector2{actionsRect.x + 14.f, actionLayout.tipY},
                   13.f, 0.f, TEXT_MUTED);
    }
#else
    (void)state;
    (void)logRect;
    (void)actionsRect;
#endif
}

void GUIView::drawPropertyPanel(const GameStateView& state, Rectangle rect) {
#if NIMONSPOLY_ENABLE_RAYLIB
    AssetManager& am = AssetManager::get();

    auto hasPlayer = [&](const std::string& username) {
        for (const PlayerView& player : state.players) {
            if (player.username == username) {
                return true;
            }
        }
        return false;
    };

    if (propertyPanelOwner_.empty() || !hasPlayer(propertyPanelOwner_)) {
        propertyPanelOwner_ = state.currentPlayerName;
    }

    const std::string ownerName = propertyPanelOwner_.empty()
        ? state.currentPlayerName
        : propertyPanelOwner_;

    drawPanelFrame(am,
                   rect,
                   "Properti Pemain",
                   ownerName.empty() ? "Tidak ada pemilik aktif" : ownerName,
                   SURFACE_BG);

    const Rectangle body = panelBodyRect(rect, 56.f, 14.f);
    std::vector<const PropertyView*> myProperties;
    for (const auto& property : state.properties) {
        if (property.ownerName == ownerName) {
            myProperties.push_back(&property);
        }
    }

    if (myProperties.empty()) {
        const std::string emptyMessage = ownerName.empty()
            ? "Belum ada properti yang dimiliki pemain aktif."
            : ("Belum ada properti yang dimiliki " + ownerName + ".");
        DrawTextEx(am.font("regular"),
                   emptyMessage.c_str(),
                   Vector2{body.x, body.y + 10.f},
                   16.f,
                   0.f,
                   TEXT_MUTED);
        return;
    }

    const float cardH = std::min(138.f, body.height - 6.f);
    const float cardW = std::clamp(cardH * 0.76f, 108.f, 132.f);
    const float gap = 12.f;
    const float totalContentW = static_cast<float>(myProperties.size()) * (cardW + gap) + gap;

    const int maxScroll = static_cast<int>(std::max(0.f, totalContentW - body.width));
    if (CheckCollisionPointRec(GetMousePosition(), body)) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.f) {
            propertyPanelScrollPx_ -= static_cast<int>(wheel * 40.f);
        }
    }
    propertyPanelScrollPx_ = std::clamp(propertyPanelScrollPx_, 0, maxScroll);

    BeginScissorMode(static_cast<int>(body.x),
                     static_cast<int>(body.y),
                     static_cast<int>(body.width),
                     static_cast<int>(body.height));

    float x = body.x - static_cast<float>(propertyPanelScrollPx_);
    for (size_t idx = 0; idx < myProperties.size(); ++idx) {
        const auto* property = myProperties[idx];
        const Rectangle card{x, body.y, cardW, cardH};
        if (x + cardW > body.x && x < body.x + body.width) {
            gui::draw::drawPanel(card, SURFACE_ALT, gui::menu::makeColor(0x40, 0x55, 0x73));

            ::Color group = ::Color::DEFAULT;
            std::string tileName = property->name;
            for (const auto& tile : state.tiles) {
                if (tile.code == property->code) {
                    group = tile.color;
                    tileName = tile.name;
                    break;
                }
            }

            const RaylibColor stripColor = gui::draw::tileColor(group);
            if (group != ::Color::DEFAULT) {
                DrawRectangleRec(Rectangle{card.x, card.y, card.width, 16.f}, stripColor);
            }

            DrawTextEx(am.font("bold"),
                       property->code.c_str(),
                       Vector2{card.x + 10.f, card.y + 26.f},
                       26.f,
                       0.f,
                       group == ::Color::YELLOW ? gui::menu::makeColor(0x90, 0x87, 0x00)
                                                : (group != ::Color::DEFAULT ? stripColor : TEXT_PRIMARY));

            auto nameLines = gui::draw::wrapText(am, "regular", tileName, 14.f, card.width - 20.f);
            float nameY = card.y + 58.f;
            for (size_t lineIdx = 0; lineIdx < nameLines.size() && lineIdx < 2; ++lineIdx) {
                DrawTextEx(am.font("regular"),
                           nameLines[lineIdx].c_str(),
                           Vector2{card.x + 10.f, nameY},
                           14.f,
                           0.f,
                           TEXT_PRIMARY);
                nameY += 14.f;
            }

            const int level = std::min(property->buildingLevel, 5);
            if (level > 0) {
                const float dotY = card.y + card.height - 18.f;
                const float dotR = 3.6f;
                const float startX = card.x + card.width * 0.5f - (level - 1) * dotR * 1.7f;
                for (int d = 0; d < level; ++d) {
                    DrawCircleV(Vector2{startX + d * dotR * 3.4f, dotY},
                                dotR,
                                property->status == PropertyStatus::MORTGAGED
                                    ? gui::menu::makeColor(150, 150, 150)
                                    : TEXT_GREEN);
                }
            }

            if (property->status == PropertyStatus::MORTGAGED) {
                const Rectangle mortRect{card.x + card.width - 56.f, card.y + 24.f, 44.f, 20.f};
                gui::draw::drawPanel(mortRect,
                                     gui::menu::makeColor(0xff, 0x4d, 0x4d, 28),
                                     gui::menu::makeColor(0xff, 0x4d, 0x4d, 110));
                drawTextCentered(am.font("bold"), "MRTG", 12.f, gui::menu::makeColor(0xff, 0xc4, 0xc4), mortRect);
            }

            if (property->festivalTurnsRemaining > 0) {
                const Rectangle festRect{card.x + 8.f, card.y + card.height - 30.f, card.width - 16.f, 18.f};
                gui::draw::drawPanel(festRect,
                                     gui::menu::makeColor(0xff, 0xd4, 0x66, 24),
                                     gui::menu::makeColor(0xff, 0xd4, 0x66, 90));
                const std::string festLabel = "FEST x" + std::to_string(property->festivalMultiplier) +
                                              " (" + std::to_string(property->festivalTurnsRemaining) + "t)";
                drawTextCentered(am.font("bold"), festLabel, 11.f, gui::menu::makeColor(0xff, 0xe8, 0xb0), festRect);
            }
        }
        x += cardW + gap;
    }

    EndScissorMode();

    if (maxScroll > 0) {
        const float trackY = body.y + body.height - 4.f;
        DrawRectangleRounded(Rectangle{body.x, trackY, body.width, 3.f},
                             1.f, 2, gui::menu::makeColor(255, 255, 255, 24));
        const float thumbW = std::max(24.f, body.width * (body.width / totalContentW));
        const float travel = std::max(0.f, body.width - thumbW);
        const float t = (maxScroll == 0) ? 0.f : static_cast<float>(propertyPanelScrollPx_) / static_cast<float>(maxScroll);
        DrawRectangleRounded(Rectangle{body.x + travel * t, trackY, thumbW, 3.f},
                             1.f, 2, gui::menu::makeColor(0x7d, 0xb4, 0xec, 220));
    }
#else
    (void)state;
    (void)rect;
#endif
}
#endif

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
    if (queuedManualDice_) {
        diceAnimFace1_ = queuedDice1_;
        diceAnimFace2_ = queuedDice2_;
    } else if (static_cast<int>(diceAnimElapsed_ * 10.f) % 2 == 0) {
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
            const std::string line = std::to_string(i + 1) + ". " + prompt.options[static_cast<size_t>(i)];
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
    const InGameLayout layout = makeInGameLayout(W, H);
    const ActionLayout actionLayout = makeActionLayout(layout.actionsPanel);
    const PlayerView* activePlayer = activePlayerView(state);
    const bool activeJailed = activePlayer && activePlayer->status == PlayerStatus::JAILED;
    const bool setDiceDisabled = state.hasRolledDice || state.extraRollAvailable || activeJailed;

    for (size_t i = 0; i < ACTION_LABELS.size(); ++i) {
        if (CheckCollisionPointRec(Vector2{mx, my}, actionLayout.buttons[i])) {
            outCommand = ACTION_LABELS[i];
            return;
        }
    }

    if (CheckCollisionPointRec(Vector2{mx, my}, actionLayout.setDiceRect)) {
        if (!setDiceDisabled && !diceAnimating_) {
            outCommand = "ATUR_DADU";
        }
        return;
    }

    if (CheckCollisionPointRec(Vector2{mx, my}, actionLayout.diceRect)) {
        if (!state.hasRolledDice && !diceAnimating_) {
            diceAnimating_ = true;
            diceAnimElapsed_ = 0.f;
            outCommand = "DADU";
        }
        return;
    }

    if (inspectedTileIndex_ >= 0) {
        const float panelW = std::min(W * 0.70f, 760.f);
        const float panelH = std::min(H * 0.55f, 460.f);
        const Rectangle panel = gui::draw::centeredRect(W * 0.5f, H * 0.5f, panelW, panelH);
        if (!CheckCollisionPointRec(Vector2{mx, my}, panel)) {
            inspectedTileIndex_ = -1;
            return;
        }
    }

    if (CheckCollisionPointRec(Vector2{mx, my}, layout.boardBounds)) {
        const int totalTiles = static_cast<int>(state.tiles.size());
        for (int i = 0; i < totalTiles; ++i) {
            const Rectangle tileBounds = gui::tile::boardTileBounds(i, layout.boardBounds, totalTiles);
            if (CheckCollisionPointRec(Vector2{mx, my}, tileBounds)) {
                inspectedTileIndex_ = i;
                return;
            }
        }
    }

    const Rectangle playersBody = panelBodyRect(layout.playersPanel, 56.f, 14.f);
    constexpr float rowGap = 10.f;
    constexpr float rowH = 62.f;
    float y = playersBody.y;
    for (size_t i = 0; i < state.players.size(); ++i) {
        const Rectangle row{playersBody.x, y, playersBody.width, rowH};
        if (CheckCollisionPointRec(Vector2{mx, my}, row)) {
            propertyPanelOwner_ = state.players[i].username;
            return;
        }
        y += rowH + rowGap;
    }
#else
    (void)mx;
    (void)my;
    (void)outCommand;
    (void)state;
#endif
}
