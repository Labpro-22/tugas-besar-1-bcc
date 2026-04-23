#include "ui/GUIView.hpp"

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "components/GUIViewDraw.hpp"
#endif

[[maybe_unused]] static constexpr float LP_W_FRAC  = 0.16f;
[[maybe_unused]] static constexpr float RP_W_FRAC  = 0.16f;
[[maybe_unused]] static constexpr float BOT_H_FRAC = 0.20f;

#if NIMONSPOLY_ENABLE_SFML
// Fixed layout constants for in-game screen (window 1440x1024)
static constexpr float LAYOUT_PAD = 2.f;      // spacing for globe / board area
static constexpr float SECTION_PAD = 2.f;     // compact margin for section cards
static constexpr float SQUARE_CARD_W = 360.f;
static constexpr float SQUARE_CARD_H = 358.f;
static constexpr float RECT_CARD_W = 1100.f;
static constexpr float RECT_CARD_H = 180.f;
static constexpr float GLOBE_SIZE = 700.f;
#endif

void GUIView::showBoard(const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;

    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    const float W = static_cast<float>(rw.getSize().x);

    // Fixed card positions (compact SECTION_PAD for cards)
    const float leftCardX = SECTION_PAD;
    const float rightCardX = W - SECTION_PAD - SQUARE_CARD_W;
    const float leftCardsBottom = SECTION_PAD + SQUARE_CARD_H + SECTION_PAD + SQUARE_CARD_H;
    const float globeBottom = LAYOUT_PAD + GLOBE_SIZE;
    const float rectCardY = std::max(leftCardsBottom, globeBottom) + SECTION_PAD;

    // Board area for tiles (between fixed cards, above bottom rect)
    const float boardLeft   = leftCardX + SQUARE_CARD_W + LAYOUT_PAD;
    const float boardRight  = rightCardX - LAYOUT_PAD;
    const float boardTop    = LAYOUT_PAD;
    const float boardBottom = rectCardY;

    const float cW = boardRight - boardLeft;
    const float cH = boardBottom - boardTop;

    // const float boardSz = std::min(cW, cH) * 0.97f;
    // const float boardSz = std::min(cW, cH) * 1.f;
    const float boardSz = 700.f;
    const sf::Vector2f origin{
        boardLeft + (cW - boardSz) * 0.5f,
        boardTop + (cH - boardSz) * 0.5f,
    };

    constexpr int EDGE  = 10;
    constexpr int SIDE  = EDGE + 1;
    constexpr int TOTAL = EDGE * 4;
    const float tileSz  = boardSz / static_cast<float>(SIDE);

    auto tilePos = [&](int idx) -> sf::Vector2f {
        const float edge = tileSz * static_cast<float>(SIDE);
        const int   sd   = idx / EDGE;
        const int   off  = idx % EDGE;
        switch (sd) {
            case 0:  return {origin.x + edge - tileSz - off * tileSz, origin.y + edge - tileSz};
            case 1:  return {origin.x,                                 origin.y + edge - tileSz - off * tileSz};
            case 2:  return {origin.x + off * tileSz,                  origin.y};
            default: return {origin.x + edge - tileSz,                 origin.y + off * tileSz};
        }
    };

    rw.clear(sf::Color(255, 255, 255));

    // Globe background
    const sf::Texture* globeTex = am.texture("assets/bg/GlobeWShadow.png");
    if (globeTex) {
        const float globeX = (W - GLOBE_SIZE) * 0.5f;
        const float globeY = LAYOUT_PAD;
        gui::draw::drawSprite(rw, globeTex, {{globeX, globeY}, {GLOBE_SIZE, GLOBE_SIZE}});
    } else {
        gui::draw::drawGameBackground(rw);
    }

    // No title above globe (as requested)

    const int numTiles = std::min<int>(TOTAL, static_cast<int>(state.tiles.size()));
    for (int i = 0; i < numTiles; ++i)
        gui::draw::drawTileCard(rw, tilePos(i), tileSz, state.tiles[static_cast<size_t>(i)], am);

    const float tokenR       = tileSz * 0.16f;
    const float tokenSpacing = tokenR * 2.4f;

    std::array<std::vector<int>, TOTAL> playersAt{};
    for (int p = 0; p < static_cast<int>(state.players.size()); ++p) {
        const auto& pv = state.players[static_cast<size_t>(p)];
        if (pv.status != PlayerStatus::BANKRUPT) {
            int pos = pv.position;
            if (pos >= 0 && pos < TOTAL)
                playersAt[static_cast<size_t>(pos)].push_back(p);
        }
    }
    for (int i = 0; i < TOTAL; ++i) {
        const auto& ps = playersAt[static_cast<size_t>(i)];
        if (ps.empty()) continue;
        const sf::Vector2f base = tilePos(i);
        const float cx = base.x + tileSz * 0.5f;
        const float cy = base.y + tileSz * 0.5f;
        const float totalW = static_cast<float>(ps.size()) * tokenSpacing;
        float startX = cx - totalW * 0.5f + tokenSpacing * 0.5f;
        for (int playerIdx : ps) {
            sf::CircleShape token(tokenR);
            token.setFillColor(gui::draw::tokenColor(playerIdx));
            token.setOutlineThickness(2.f);
            token.setOutlineColor(sf::Color(255, 255, 255, 200));
            token.setOrigin({tokenR, tokenR});
            token.setPosition({startX, cy});
            rw.draw(token);
            startX += tokenSpacing;
        }
    }

    drawLeftPanel (rw, state);
    drawRightPanel(rw, state);
    drawBottomStrip(rw, state);

    if (currentPrompt_ && currentPrompt_->type != GUIPromptType::NONE && !currentPrompt_->resolved) {
        renderPromptOverlay(*currentPrompt_);
    }

    rw.display();
#else
    (void)rw; (void)state;
#endif
}

void GUIView::renderPromptOverlay(const GUIPromptState& prompt) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;

    // Dim background
    sf::RectangleShape dim({W, H});
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    rw.draw(dim);

    // Panel
    const float panelW = W * 0.42f;
    const float panelH = H * 0.28f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({cx - panelW * 0.5f, cy - panelH * 0.5f});
    panel.setFillColor(sf::Color(250, 250, 252));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(80, 130, 200));
    rw.draw(panel);

    unsigned titleSz = static_cast<unsigned>(panelH * 0.13f);
    unsigned bodySz  = static_cast<unsigned>(panelH * 0.10f);
    unsigned hintSz  = static_cast<unsigned>(panelH * 0.07f);

    // Title / label
    sf::Text title(am.font("bold"), prompt.label, titleSz);
    title.setFillColor(sf::Color(35, 55, 90));
    auto tb = title.getLocalBounds();
    title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y});
    title.setPosition({cx, cy - panelH * 0.30f});
    rw.draw(title);

    // Type-specific content
    float contentY = cy - panelH * 0.08f;

    if (prompt.type == GUIPromptType::MENU_CHOICE) {
        float optY = contentY;
        for (int i = 0; i < static_cast<int>(prompt.options.size()); ++i) {
            std::string line = std::to_string(i) + ". " + prompt.options[static_cast<size_t>(i)];
            sf::Text opt(am.font("regular"), line, bodySz);
            opt.setFillColor(sf::Color(60, 80, 120));
            auto ob = opt.getLocalBounds();
            opt.setOrigin({ob.position.x + ob.size.x * 0.5f, ob.position.y});
            opt.setPosition({cx, optY});
            rw.draw(opt);
            optY += bodySz * 1.4f;
        }
    } else if (prompt.type == GUIPromptType::YES_NO) {
        sf::Text hint(am.font("regular"), "Ketik Y / N lalu Enter", hintSz);
        hint.setFillColor(sf::Color(120, 140, 170));
        auto hb = hint.getLocalBounds();
        hint.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
        hint.setPosition({cx, contentY});
        rw.draw(hint);
    } else if (prompt.type == GUIPromptType::AUCTION) {
        sf::Text hint(am.font("regular"), "Ketik PASS atau BID <jumlah>", hintSz);
        hint.setFillColor(sf::Color(120, 140, 170));
        auto hb = hint.getLocalBounds();
        hint.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
        hint.setPosition({cx, contentY});
        rw.draw(hint);
    } else {
        sf::Text hint(am.font("regular"), "Ketik jawaban lalu Enter", hintSz);
        hint.setFillColor(sf::Color(120, 140, 170));
        auto hb = hint.getLocalBounds();
        hint.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
        hint.setPosition({cx, contentY});
        rw.draw(hint);
    }

    // Text input buffer
    sf::Text buffer(am.font("regular"), "> " + prompt.textBuffer + "_", bodySz);
    buffer.setFillColor(sf::Color(40, 60, 100));
    auto bb = buffer.getLocalBounds();
    buffer.setOrigin({bb.position.x + bb.size.x * 0.5f, bb.position.y});
    buffer.setPosition({cx, cy + panelH * 0.22f});
    rw.draw(buffer);
#endif
}

void GUIView::handleInGameClick(float mx, float my, std::string& outCommand, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    const float W = static_cast<float>(window->getSize().x);
    const float H = static_cast<float>(window->getSize().y);

    const float rpX = W - SECTION_PAD - SQUARE_CARD_W;
    const float colW = SQUARE_CARD_W;

    // Calculate button positions (same as drawRightPanel)
    float y = SECTION_PAD + SQUARE_CARD_H + SECTION_PAD;

    // Action buttons: Buy, Build, Mortgage, Card
    float btnSz = std::min(colW * 0.46f, (H - y - SECTION_PAD - 100.f) / 2.5f);
    float gap = btnSz * 0.18f;
    float gridW = btnSz * 2.f + gap;
    float startX = rpX + (colW - gridW) * 0.5f + btnSz * 0.5f;
    float startY = y + btnSz * 0.5f;

    const char* actions[] = {"BELI", "BANGUN", "GADAI", "KARTU"};
    for (int i = 0; i < 4; ++i) {
        int col = i % 2;
        int row = i / 2;
        float bx = startX + col * (btnSz + gap);
        float by = startY + row * (btnSz + gap);
        float half = btnSz * 0.5f;
        if (mx >= bx - half && mx <= bx + half &&
            my >= by - half && my <= by + half) {
            outCommand = actions[i];
            return;
        }
    }

    // "Lempar Dadu" button
    float btnW = colW;
    float btnH = std::min(H * 0.14f, 100.f);
    float bx = rpX + colW * 0.5f;
    float by = H - SECTION_PAD - btnH * 0.5f;
    if (mx >= bx - btnW * 0.5f && mx <= bx + btnW * 0.5f &&
        my >= by - btnH * 0.5f && my <= by + btnH * 0.5f) {
        if (!state.hasRolledDice) {
            outCommand = "DADU";
        }
    }
#endif
}
void GUIView::drawLeftPanel(sf::RenderWindow& rw, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    (void)rw;

    const float colX = SECTION_PAD;
    const float colW = SQUARE_CARD_W;
    const float pad  = colW * 0.03f;

    unsigned bodySz = static_cast<unsigned>(colW * 0.05f);
    unsigned smSz   = static_cast<unsigned>(colW * 0.04f);

    const float sumCardY = SECTION_PAD;
    const float sumCardH = SQUARE_CARD_H;
    const float plrCardY = sumCardY + sumCardH + SECTION_PAD;
    const float plrCardH = SQUARE_CARD_H;

    const sf::Texture* sumTex = am.texture("assets/components/card/SquareCard.png");
    const sf::Texture* plrTex = am.texture("assets/components/card/SquareCard.png");

    if (sumTex) {
        gui::draw::drawSprite(rw, sumTex, {{colX, sumCardY}, {colW, sumCardH}});
    } else {
        gui::draw::drawPanel(rw, {{colX, sumCardY}, {colW, sumCardH}}, sf::Color(0x17, 0x28, 0x48, 200));
    }
    {
        unsigned hSz2 = static_cast<unsigned>(colW * 0.06f);
        sf::Text t(am.font("title"), "GAME SUMMARY", hSz2);
        t.setFillColor(sf::Color(45, 65, 100));
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
        t.setPosition({colX + colW * 0.5f, sumCardY + pad * 1.2f});
        rw.draw(t);
    }

    if (plrTex) {
        gui::draw::drawSprite(rw, plrTex, {{colX, plrCardY}, {colW, plrCardH}});
    } else {
        gui::draw::drawPanel(rw, {{colX, plrCardY}, {colW, plrCardH}}, sf::Color(0x10, 0x1c, 0x38, 230));
    }
    {
        unsigned hSz2 = static_cast<unsigned>(colW * 0.06f);
        sf::Text t(am.font("title"), "PLAYERS", hSz2);
        t.setFillColor(sf::Color(45, 65, 100));
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
        t.setPosition({colX + colW * 0.5f, plrCardY + pad * 1.2f});
        rw.draw(t);
    }

    {
        float y = sumCardY + sumCardH * 0.22f;

        std::string roundStr = "Round  " + std::to_string(state.currentTurn)
                             + " / " + std::to_string(state.maxTurn);
        gui::draw::drawLabel(rw, am, "regular", roundStr, bodySz,
                             sf::Color(55, 75, 110), {colX + pad, y});
        y += bodySz * 1.8f;

        const sf::Color tcols[] = {
            sf::Color(0x00,0xc8,0xff), sf::Color(0xff,0x2d,0x8a),
            sf::Color(0xff,0xf2,0x00), sf::Color(0x00,0xff,0xb0)
        };
        float tx   = colX + pad;
        float ty   = y + smSz * 0.5f;
        float dotR = smSz * 0.55f;
        for (int i = 0; i < static_cast<int>(state.players.size()) && i < 4; ++i) {
            bool bankrupt = state.players[static_cast<size_t>(i)].status == PlayerStatus::BANKRUPT;
            sf::CircleShape dot(dotR);
            dot.setFillColor(bankrupt ? sf::Color(160, 160, 170) : tcols[i % 4]);
            dot.setOrigin({dotR, dotR});
            dot.setPosition({tx + dotR, ty});
            rw.draw(dot);
            std::string nm  = state.players[static_cast<size_t>(i)].username;
            std::string ini = nm.empty() ? "?" : std::string(1, nm[0]);
            sf::Text t(am.font("bold"), ini, static_cast<unsigned>(dotR * 1.1f));
            t.setFillColor(sf::Color(0, 0, 0, bankrupt ? 80 : 220));
            auto b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
            t.setPosition({tx + dotR, ty});
            rw.draw(t);
            tx += dotR * 2.8f;
        }
    }

    {
        float contentY = plrCardY + plrCardH * 0.22f;
        float y = contentY;

        const sf::Color tcols[] = {
            sf::Color(0x00,0xc8,0xff), sf::Color(0xff,0x2d,0x8a),
            sf::Color(0xff,0xf2,0x00), sf::Color(0x00,0xff,0xb0)
        };
        float rowH = (plrCardY + plrCardH - pad - contentY)
               / std::max(1, static_cast<int>(state.players.size()));
        rowH = std::min(rowH, plrCardH * 0.18f);

        for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
            const auto& pv = state.players[static_cast<size_t>(i)];
            bool active   = (pv.username == state.currentPlayerName);
            bool bankrupt = (pv.status == PlayerStatus::BANKRUPT);

            if (active) {
                sf::RectangleShape hl({colW - pad * 0.5f, rowH - pad * 0.2f});
                hl.setPosition({colX + pad * 0.25f, y});
                hl.setFillColor(sf::Color(tcols[i % 4].r, tcols[i % 4].g, tcols[i % 4].b, 40));
                hl.setOutlineThickness(1.f);
                hl.setOutlineColor(sf::Color(tcols[i % 4].r, tcols[i % 4].g, tcols[i % 4].b, 100));
                rw.draw(hl);
            }

            float dotR = rowH * 0.22f;
            sf::CircleShape dot(dotR);
            dot.setFillColor(bankrupt ? sf::Color(160, 160, 170) : tcols[i % 4]);
            dot.setOrigin({dotR, dotR});
            dot.setPosition({colX + pad + dotR, y + rowH * 0.3f});
            rw.draw(dot);

            sf::Color nameCol = bankrupt ? sf::Color(140, 145, 155)
                              : active   ? sf::Color(20, 35, 60)
                                         : sf::Color(50, 70, 100);
            std::string displayName = pv.username;
            if (displayName.size() > 14) displayName = displayName.substr(0, 12) + "..";
            gui::draw::drawLabel(rw, am, "bold", displayName, bodySz, nameCol,
                                 {colX + pad + dotR * 2.8f, y + rowH * 0.08f});

            std::string moneyStr = "M" + std::to_string(pv.money.getAmount());
            gui::draw::drawLabel(rw, am, "regular", moneyStr, smSz,
                                 bankrupt ? sf::Color(140, 145, 155) : sf::Color(20, 110, 70),
                                 {colX + pad + dotR * 2.8f, y + rowH * 0.52f});

            std::string propStr = std::to_string(pv.propertyCount) + " prop";
            gui::draw::drawLabel(rw, am, "regular", propStr, smSz,
                                 sf::Color(90, 110, 145),
                                 {colX + pad + dotR * 2.8f + colW * 0.28f, y + rowH * 0.52f});

            if (bankrupt) {
                gui::draw::drawLabel(rw, am, "bold", "BANKRUPT", smSz,
                                     sf::Color(180, 50, 50), {colX + pad, y + rowH * 0.75f});
            }

            y += rowH;
        }
    }
#else
    (void)rw; (void)state;
#endif
}

void GUIView::drawRightPanel(sf::RenderWindow& rw, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    const float W   = static_cast<float>(rw.getSize().x);
    const float H   = static_cast<float>(rw.getSize().y);

    const float rpX = W - SECTION_PAD - SQUARE_CARD_W;
    const float colW = SQUARE_CARD_W;
    const float padX = colW * 0.03f;
    const float padY = colW * 0.03f;

    unsigned hSz  = static_cast<unsigned>(colW * 0.06f);
    unsigned smSz = static_cast<unsigned>(colW * 0.04f);

    float y = SECTION_PAD;

    {
        float logH = SQUARE_CARD_H;
        const sf::Texture* logCardTex = am.texture("assets/components/card/SquareCard.png");
        if (logCardTex) {
            gui::draw::drawSprite(rw, logCardTex, {{rpX, y}, {colW, logH}});
        } else {
            gui::draw::drawPanel(rw, {{rpX, y}, {colW, logH}},
                                 sf::Color(0x0d, 0x1a, 0x30, 230));
        }
        {
            sf::Text t(am.font("title"), "GAME LOG", hSz);
            t.setFillColor(sf::Color(45, 65, 100));
            auto b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
            t.setPosition({rpX + colW * 0.5f, y + padY * 1.2f});
            rw.draw(t);
        }

        const float logPad = colW * 0.03f;
        float logContentY = y + logH * 0.20f;
        float lineH    = smSz * 1.45f;

        int total = static_cast<int>(log_.size());

        struct LogEntryLines { int index; std::vector<std::string> lines; };
        std::vector<LogEntryLines> visible;
        float usedH = 0;
        for (int li = total - 1; li >= 0; --li) {
            const auto& e = log_[static_cast<size_t>(li)];
            std::string fullLine = "[" + e.username + "] " + e.detail;
            auto wrapped = gui::draw::wrapText(am, "regular", fullLine, smSz, colW - 2*logPad);
            float h = wrapped.size() * lineH;
            if (usedH + h > logH * 0.83f) break;
            visible.push_back({li, wrapped});
            usedH += h;
        }

        float drawY = logContentY;
        for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
            sf::Color lc = (it->index == total - 1) ? sf::Color(25, 45, 85)
                                                     : sf::Color(70, 95, 135);
            for (const auto& wl : it->lines) {
                gui::draw::drawLabel(rw, am, "regular", wl, smSz, lc,
                                     {rpX + logPad, drawY});
                drawY += lineH;
            }
        }
        if (log_.empty()) {
            gui::draw::drawLabel(rw, am, "regular", "No events yet", smSz,
                                 sf::Color(90, 115, 155),
                                 {rpX + logPad, logContentY});
        }
        y += logH + SECTION_PAD;
    }

    {
        const char* actions[] = {"Buy", "Build", "Mortgage", "Card"};
        const sf::Texture* squareBtnTex = am.texture("assets/components/btn/EmptySquareBtn.png");
        float btnSz = std::min(colW * 0.46f, (H - y - SECTION_PAD - 100.f) / 2.5f);
        float gap = btnSz * 0.18f;
        float gridW = btnSz * 2.f + gap;
        float startX = rpX + (colW - gridW) * 0.5f + btnSz * 0.5f;
        float startY = y + btnSz * 0.5f;
        for (int i = 0; i < 4; ++i) {
            int col = i % 2;
            int row = i / 2;
            float bx = startX + col * (btnSz + gap);
            float by = startY + row * (btnSz + gap);
            if (squareBtnTex) {
                gui::draw::drawSprite(rw, squareBtnTex,
                                      {{bx - btnSz * 0.5f, by - btnSz * 0.5f}, {btnSz, btnSz}});
                unsigned csz = static_cast<unsigned>(btnSz * 0.24f);
                sf::Text t(am.font("bold"), actions[i], csz);
                t.setFillColor(sf::Color(45, 65, 100));
                auto b = t.getLocalBounds();
                t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
                t.setPosition({bx, by});
                rw.draw(t);
            } else {
                gui::draw::drawMenuButton(rw, am, actions[i], {bx, by}, {btnSz, btnSz}, false);
            }
        }
        y += 2.f * (btnSz + gap) + padY * 0.6f;
    }

    {
        float btnW = colW;
        float btnH = std::min(H * 0.14f, 100.f);
        float bx   = rpX + colW * 0.5f;
        float by   = H - LAYOUT_PAD - btnH * 0.5f;
        bool disabled = state.hasRolledDice;

        const sf::Texture* rectBtnTex = am.texture("assets/components/btn/EmptyRectBtn.png");
        if (rectBtnTex && !disabled) {
            gui::draw::drawSprite(rw, rectBtnTex,
                                  {{bx - btnW * 0.5f, by - btnH * 0.5f}, {btnW, btnH}});
        } else {
            sf::RectangleShape btn({btnW, btnH});
            btn.setOrigin({btnW * 0.5f, btnH * 0.5f});
            btn.setPosition({bx, by});
            btn.setFillColor(disabled ? sf::Color(0xd0, 0xd8, 0xe4, 180)
                                      : sf::Color(0xb8, 0xcc, 0xe0));
            btn.setOutlineThickness(1.5f);
            btn.setOutlineColor(disabled ? sf::Color(160, 170, 190)
                                         : sf::Color(80, 130, 200));
            rw.draw(btn);
        }

        unsigned tsz = static_cast<unsigned>(btnH * 0.36f);
        sf::Text t(am.font("title"), "Lempar Dadu", tsz);
        t.setFillColor(disabled ? sf::Color(145, 155, 175) : sf::Color(25, 45, 85));
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({bx, by});
        rw.draw(t);
    }

    (void)state;
#else
    (void)rw; (void)state;
#endif
}

void GUIView::drawBottomStrip(sf::RenderWindow& rw, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    (void)rw;

    const float leftCardsBottom = SECTION_PAD + SQUARE_CARD_H + SECTION_PAD + SQUARE_CARD_H;
    const float globeBottom = LAYOUT_PAD + GLOBE_SIZE;
    const float stripY = std::max(leftCardsBottom, globeBottom) + SECTION_PAD;
    const float stripX = SECTION_PAD;
    const float stripW = RECT_CARD_W;
    const float botH   = RECT_CARD_H;
    const float pad    = botH * 0.08f;

    const sf::Texture* propCardTex = am.texture("assets/components/card/RectCard.png");
    if (propCardTex) {
        gui::draw::drawSprite(rw, propCardTex, {{stripX, stripY}, {stripW, botH}});
    } else {
        gui::draw::drawPanel(rw, {{stripX, stripY}, {stripW, botH}}, sf::Color(0x0f, 0x1a, 0x32, 220));
    }
    {
        unsigned hSz2 = static_cast<unsigned>(botH * 0.07f);
        sf::Text t(am.font("title"), "YOUR PROPERTY", hSz2);
        t.setFillColor(sf::Color(45, 65, 100));
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
        t.setPosition({stripX + stripW * 0.5f, stripY + pad * 1.2f});
        rw.draw(t);
    }

    float cardY = stripY + botH * 0.28f;
    const float cardH   = botH - (botH * 0.24f) - pad;
    const float cardW   = cardH * 0.72f;
    const float cardGap = cardW * 0.18f;
    float cx   = stripX + pad;
    float maxX = stripX + stripW - pad;

    std::vector<const PropertyView*> myProps;
    for (const auto& pv : state.properties) {
        if (pv.ownerName == state.currentPlayerName)
            myProps.push_back(&pv);
    }

    if (myProps.empty()) {
        gui::draw::drawLabel(rw, am, "regular", "No properties owned",
                             static_cast<unsigned>(botH * 0.15f),
                             sf::Color(80, 100, 130), {cx, cardY + cardH * 0.3f});
    }

    for (const auto* pv : myProps) {
        if (cx + cardW > maxX) break;

        sf::RectangleShape card({cardW, cardH});
        card.setPosition({cx, cardY});
        card.setFillColor(sf::Color(250, 250, 248));
        card.setOutlineThickness(1.f);
        card.setOutlineColor(sf::Color(18, 20, 25));
        rw.draw(card);

        Color col = Color::DEFAULT;
        for (const auto& tv : state.tiles) {
            if (tv.code == pv->code) { col = tv.color; break; }
        }
        sf::Color sc = gui::draw::tileColor(col);
        if (col != Color::DEFAULT) {
            sf::RectangleShape strip({cardW, cardH * 0.18f});
            strip.setPosition({cx, cardY});
            strip.setFillColor(sc);
            rw.draw(strip);
        }

        unsigned csz = static_cast<unsigned>(cardH * 0.20f);
        sf::Text ct(am.font("bold"), pv->code, csz);
        ct.setFillColor(col != Color::DEFAULT ? sc : sf::Color(80, 90, 110));
        if (col == Color::YELLOW) ct.setFillColor(sf::Color(0x90, 0x87, 0x00));
        auto b = ct.getLocalBounds();
        ct.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        ct.setPosition({cx + cardW * 0.5f, cardY + cardH * 0.60f});
        rw.draw(ct);

        if (pv->buildingLevel > 0) {
            float dotR  = cardH * 0.055f;
            float dotY  = cardY + cardH * 0.84f;
            int   lvl   = std::min(pv->buildingLevel, 5);
            float startDX = cx + cardW * 0.5f - (lvl - 1) * dotR * 1.4f;
            for (int d = 0; d < lvl; ++d) {
                sf::CircleShape dot(dotR);
                dot.setFillColor(pv->status == PropertyStatus::MORTGAGED
                                 ? sf::Color(150, 150, 150) : sf::Color(0x00, 0x93, 0x64));
                dot.setOrigin({dotR, dotR});
                dot.setPosition({startDX + d * dotR * 2.8f, dotY});
                rw.draw(dot);
            }
        }

        if (pv->status == PropertyStatus::MORTGAGED) {
            sf::RectangleShape ov({cardW, cardH});
            ov.setPosition({cx, cardY});
            ov.setFillColor(sf::Color(0, 0, 0, 100));
            rw.draw(ov);
            unsigned msz = static_cast<unsigned>(cardH * 0.14f);
            sf::Text mt(am.font("bold"), "MRTG", msz);
            mt.setFillColor(sf::Color(220, 80, 80));
            auto mb2 = mt.getLocalBounds();
            mt.setOrigin({mb2.position.x + mb2.size.x * 0.5f,
                          mb2.position.y + mb2.size.y * 0.5f});
            mt.setPosition({cx + cardW * 0.5f, cardY + cardH * 0.5f});
            rw.draw(mt);
        }

        cx += cardW + cardGap;
    }
#else
    (void)rw; (void)state;
#endif
}
