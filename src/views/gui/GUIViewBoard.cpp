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

void GUIView::showBoard(const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;

    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float lpW  = W * LP_W_FRAC;
    const float rpW  = W * RP_W_FRAC;
    const float botH = H * BOT_H_FRAC;

    const float cW = W - lpW - rpW;
    const float cH = H - botH;

    const float boardSz = std::min(cW, cH) * 0.97f;
    const sf::Vector2f origin{
        lpW + (cW - boardSz) * 0.5f,
        (cH - boardSz) * 0.5f,
    };

    constexpr int EDGE  = 10;
    constexpr int SIDE  = EDGE + 1;
    constexpr int TOTAL = EDGE * 4;
    const float tileSz  = boardSz / static_cast<float>(SIDE);
    const float innerSz = boardSz - 2.0f * tileSz;

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

    rw.clear(sf::Color(0, 0, 0));

    const sf::Texture* globeTex = am.texture("assets/bg/Globe.png");
    if (globeTex) {
        gui::draw::drawSpriteCover(rw, globeTex);
    } else {
        gui::draw::drawGameBackground(rw);
    }

    const sf::Vector2f innerCenter{origin.x + boardSz * 0.5f,
                                   origin.y + boardSz * 0.5f};

    const sf::Texture* titleTex = am.texture("assets/bg/Title.png");
    if (titleTex) {
        auto tsz2 = titleTex->getSize();
        float titleW = innerSz * 0.68f;
        float titleH = titleW * static_cast<float>(tsz2.y) / static_cast<float>(tsz2.x);
        sf::RenderStates addBlend;
        addBlend.blendMode = sf::BlendAdd;
        gui::draw::drawSprite(rw, titleTex,
                              {{innerCenter.x - titleW * 0.5f,
                                innerCenter.y - titleH * 0.58f},
                               {titleW, titleH}},
                              addBlend);
    } else {
        float logoSz = innerSz * 0.28f;
        unsigned charSz = static_cast<unsigned>(logoSz);
        sf::Text line1(am.font("title"), std::string("NIMONS"), charSz);
        sf::Text line2(am.font("title"), std::string("POLY"),   charSz);
        line1.setFillColor(sf::Color(235, 240, 248));
        line2.setFillColor(sf::Color(235, 240, 248));
        auto b1 = line1.getLocalBounds();
        auto b2 = line2.getLocalBounds();
        line1.setOrigin({b1.position.x + b1.size.x * 0.5f, b1.position.y + b1.size.y});
        line2.setOrigin({b2.position.x + b2.size.x * 0.5f, b2.position.y});
        line1.setPosition({innerCenter.x, innerCenter.y - logoSz * 0.08f});
        line2.setPosition({innerCenter.x, innerCenter.y + logoSz * 0.08f});
        rw.draw(line1);
        rw.draw(line2);
    }

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

    rw.display();
#else
    (void)state;
#endif
}

void GUIView::drawLeftPanel(sf::RenderWindow& rw, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float lpW  = W * LP_W_FRAC;
    const float botH = H * BOT_H_FRAC;
    const float panH = H - botH;
    const float colPadX = lpW * 0.07f;
    const float colPadY = lpW * 0.06f;
    const float colW = lpW - colPadX * 2.f;
    const float colH = panH - colPadY * 2.f;
    const float gapY = colPadY * 0.9f;
    const float colX = colPadX;
    const float pad  = colW * 0.07f;

    unsigned bodySz = static_cast<unsigned>(colW * 0.10f);
    unsigned smSz   = static_cast<unsigned>(colW * 0.085f);

    const float sumCardH = colH * 0.36f;
    const float plrCardH = colH - sumCardH - gapY;
    const float sumCardY = colPadY;
    const float plrCardY = sumCardY + sumCardH + gapY;

    const sf::Texture* sumTex = am.texture("assets/components/card/GameSummaryCard.png");
    const sf::Texture* plrTex = am.texture("assets/components/card/PlayerCard.png");

    if (sumTex) {
        gui::draw::drawSprite(rw, sumTex, {{colX, sumCardY}, {colW, sumCardH}});
    } else {
        gui::draw::drawPanel(rw, {{colX, sumCardY}, {colW, sumCardH}}, sf::Color(0x17, 0x28, 0x48, 200));
        unsigned hSz2 = static_cast<unsigned>(colW * 0.13f);
        gui::draw::drawLabel(rw, am, "bold", "GAME SUMMARY", hSz2,
                             sf::Color(160, 200, 240), {colX + pad, sumCardY + pad * 0.6f});
    }

    if (plrTex) {
        gui::draw::drawSprite(rw, plrTex, {{colX, plrCardY}, {colW, plrCardH}});
    } else {
        gui::draw::drawPanel(rw, {{colX, plrCardY}, {colW, plrCardH}}, sf::Color(0x10, 0x1c, 0x38, 230));
        unsigned hSz2 = static_cast<unsigned>(colW * 0.13f);
        gui::draw::drawLabel(rw, am, "bold", "PLAYERS", hSz2,
                             sf::Color(160, 200, 240), {colX + pad, plrCardY + pad * 0.6f});
    }

    {
        float y = sumCardY + sumCardH * 0.17f;

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
        float contentY = plrCardY + plrCardH * 0.17f;
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
            gui::draw::drawLabel(rw, am, "bold", pv.username, bodySz, nameCol,
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
    const float rpW = W * RP_W_FRAC;
    const float rpX = W - rpW;
    const float padX = rpW * 0.08f;
    const float padY = rpW * 0.08f;
    const float colW = rpW - padX * 2.f;

    unsigned hSz  = static_cast<unsigned>(colW * 0.09f);
    unsigned smSz = static_cast<unsigned>(colW * 0.07f);

    float y = padY;

    {
        float logH = H * 0.42f;
        const sf::Texture* logCardTex = am.texture("assets/components/card/GameLogCard.png");
        if (logCardTex) {
            gui::draw::drawSprite(rw, logCardTex, {{rpX + padX, y}, {colW, logH}});
        } else {
            gui::draw::drawPanel(rw, {{rpX + padX, y}, {colW, logH}},
                                 sf::Color(0x0d, 0x1a, 0x30, 230));
            gui::draw::drawLabel(rw, am, "bold", "GAME LOG", hSz,
                                 sf::Color(160, 200, 240), {rpX + padX + padX * 0.4f, y + padY * 0.5f});
        }

        const float logPad = colW * 0.06f;
        float logContentY = y + logH * 0.17f;
        float lineH    = smSz * 1.55f;
        int   maxLines = static_cast<int>((logH * 0.83f) / lineH);

        int total = static_cast<int>(log_.size());
        int start = std::max(0, total - maxLines - logScrollOffset_);
        int end   = std::min(total, start + maxLines);

        for (int li = start; li < end; ++li) {
            const auto& e = log_[static_cast<size_t>(li)];
            std::string line = "[" + e.username + "] " + e.detail;
            if (line.size() > 28) line = line.substr(0, 26) + "..";
            sf::Color lc = (li == total - 1) ? sf::Color(200, 225, 255)
                                              : sf::Color(140, 170, 210);
            gui::draw::drawLabel(rw, am, "regular", line, smSz, lc,
                                 {rpX + padX + logPad, logContentY + (li - start) * lineH});
        }
        if (log_.empty()) {
            gui::draw::drawLabel(rw, am, "regular", "No events yet", smSz,
                                 sf::Color(120, 150, 190),
                                 {rpX + padX + logPad, logContentY});
        }
        y += logH + padY * 0.8f;
    }

    {
        const char* actions[] = {"Buy", "Build", "Mortgage", "Card"};
        const sf::Texture* squareBtnTex = am.texture("assets/components/btn/EmptySquareBtn.png");
        float btnSz = std::min(colW * 0.46f, H * 0.12f);
        float gap = btnSz * 0.18f;
        float gridW = btnSz * 2.f + gap;
        float startX = rpX + padX + (colW - gridW) * 0.5f + btnSz * 0.5f;
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
        float btnH = H * 0.14f;
        float bx   = rpX + rpW * 0.5f;
        float by   = H - padY - btnH * 0.5f;
        bool disabled = diceRolled_;

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
    const float W    = static_cast<float>(rw.getSize().x);
    const float H    = static_cast<float>(rw.getSize().y);
    const float rpW  = W * RP_W_FRAC;
    const float botH = H * BOT_H_FRAC;
    const float stripRightPad = rpW * 0.08f;
    const float stripX = W * LP_W_FRAC * 0.07f;
    const float stripW = W - rpW - stripX - stripRightPad;
    const float stripY = H - botH;
    const float pad    = botH * 0.12f;

    const sf::Texture* propCardTex = am.texture("assets/components/card/YourPropertyCard.png");
    if (propCardTex) {
        gui::draw::drawSprite(rw, propCardTex, {{stripX, stripY}, {stripW, botH}});
    } else {
        gui::draw::drawPanel(rw, {{stripX, stripY}, {stripW, botH}}, sf::Color(0x0f, 0x1a, 0x32, 220));
        unsigned hSz2 = static_cast<unsigned>(botH * 0.18f);
        gui::draw::drawLabel(rw, am, "bold", "YOUR PROPERTY", hSz2,
                             sf::Color(160, 200, 240), {stripX + pad, stripY + pad * 0.8f});
    }

    float cardY = stripY + botH * 0.28f;
    const float cardH   = botH - (botH * 0.28f) - pad;
    const float cardW   = cardH * 0.72f;
    const float cardGap = cardW * 0.18f;
    float cx   = stripX + pad;
    float maxX = stripX + stripW - pad;

    std::vector<const PropertyView*> myProps;
    for (const auto& pv : state.properties) {
        if (pv.ownerName == state.currentPlayerName)
            myProps.push_back(&pv);
    }

    if (myProps.empty() && !propCardTex) {
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
