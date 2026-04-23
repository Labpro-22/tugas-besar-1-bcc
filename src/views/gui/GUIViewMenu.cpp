#include "ui/GUIView.hpp"

#include "ui/AssetManager.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <string>

#include "components/GUIViewDraw.hpp"

namespace {
    void drawMenuBackground(sf::RenderWindow& rw, AssetManager& am) {
        const sf::Texture* globeTex = am.texture("assets/bg/Globe.png");
        if (globeTex) {
            gui::draw::drawSpriteCover(rw, globeTex);
        } else {
            gui::draw::drawGlobeBackground(rw);
        }
    }

    void drawMenuText(sf::RenderWindow& rw, AssetManager& am,
                      const std::string& label, sf::Vector2f center,
                      unsigned size, bool hovered, bool selected = false) {
        sf::Color tc = selected ? sf::Color(255, 255, 255)
                     : hovered  ? sf::Color(235, 240, 255)
                                : sf::Color(190, 200, 220);
        sf::Text t(am.font("title"), label, size);
        t.setFillColor(tc);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition(center);
        rw.draw(t);
    }
}
#endif

void GUIView::renderCurrentScreen() {
    switch (screen_) {
        case AppScreen::LANDING:              drawLandingPage(); break;
        case AppScreen::NEW_GAME_NUM_PLAYERS: drawNumPlayers();  break;
        case AppScreen::NEW_GAME_CUST_PLAYER: drawCustPlayer();  break;
        case AppScreen::NEW_GAME_CUST_MAP:    drawCustMap();     break;
        case AppScreen::LOAD_GAME:            drawLoadGame();    break;
        case AppScreen::IN_GAME:              break;
        case AppScreen::GAME_OVER:            drawGameOver();    break;
    }
}

void GUIView::drawNumPlayers() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0, 0, 0));
    drawMenuBackground(rw, am);

    const float cx = static_cast<float>(rw.getSize().x) * 0.5f;
    const float cy = static_cast<float>(rw.getSize().y) * 0.5f;
    const float h  = static_cast<float>(rw.getSize().y);

    const float panelH = h * 0.42f;

    unsigned hSz = static_cast<unsigned>(h * 0.042f);
    drawMenuText(rw, am, "< Back",
                 {cx, cy - panelH * 0.48f}, hSz, hoveredItem_ == 20, false);
    drawMenuText(rw, am, "Choose number of players",
                 {cx, cy - panelH * 0.32f}, hSz, false, true);

    const char* opts[] = {"2 Players", "3 Players", "4 Players"};
    const float btnH = h * 0.066f;
    const float startY = cy - panelH * 0.08f;
    const float gap    = h * 0.09f;
    for (int i = 0; i < 3; ++i) {
        bool sel = (setup_.numPlayers == i + 2);
        unsigned optSz = static_cast<unsigned>(btnH * 0.92f);
        drawMenuText(rw, am, opts[i],
                 {cx, startY + i * gap}, optSz, hoveredItem_ == i, sel);
    }

        unsigned nextSz = static_cast<unsigned>(btnH * 0.9f);
        drawMenuText(rw, am, "Next >",
             {cx, cy + panelH * 0.58f}, nextSz, hoveredItem_ == 10);

    rw.display();
#endif
}

void GUIView::drawCustPlayer() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0, 0, 0));
    drawMenuBackground(rw, am);

    const float W  = static_cast<float>(rw.getSize().x);
    const float H  = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned hSz = static_cast<unsigned>(H * 0.042f);
    drawMenuText(rw, am, "< Customize Player", {cx, H * 0.10f}, hSz, false, true);

    const float tabH = H * 0.07f;
    const float tabX = W * 0.18f;
    const float tabStartY = cy - setup_.numPlayers * tabH * 0.5f;
    const sf::Color tabColors[] = {
        sf::Color(0x00, 0xc8, 0xff),
        sf::Color(0xff, 0x2d, 0x8a),
        sf::Color(0xff, 0xf2, 0x00),
        sf::Color(0x00, 0xff, 0xb0),
    };
    for (int i = 0; i < setup_.numPlayers; ++i) {
        bool sel = (i == custPlayerTab_);
        std::string label = "P" + std::to_string(i + 1);
        unsigned tsz = static_cast<unsigned>(tabH * 0.58f);
        sf::Color tc = tabColors[i % 4];
        tc.a = sel ? 255 : 140;
        sf::Text t(am.font("title"), label, tsz);
        t.setFillColor(tc);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({tabX, tabStartY + i * tabH});
        rw.draw(t);
    }

    const float panelX = cx - W * 0.02f;
    const float panelW = W * 0.42f, panelH = H * 0.60f;

    int p = custPlayerTab_;
    float formX = panelX;
    float rowY = cy - panelH * 0.35f;
    float rowStep = H * 0.1f;
    unsigned lSz = static_cast<unsigned>(H * 0.028f);

    drawMenuText(rw, am, "Name", {formX, rowY}, lSz, false, true);
    rowY += rowStep * 0.5f;
    {
        const float lineW = panelW * 0.75f;
        sf::RectangleShape line({lineW, 2.f});
        line.setPosition({formX - lineW * 0.5f, rowY + H * 0.022f});
        line.setFillColor(sf::Color(220, 230, 240, 170));
        rw.draw(line);

        std::string name = (p < static_cast<int>(setup_.playerNames.size()))
                           ? setup_.playerNames[static_cast<size_t>(p)]
                           : "";
        unsigned nameSz = static_cast<unsigned>(H * 0.03f);
        sf::Text nt(am.font("title"), name.empty() ? "Player " + std::to_string(p+1) : name,
                    nameSz);
        nt.setFillColor(name.empty() ? sf::Color(170, 180, 200) : sf::Color(240, 245, 255));
        auto b = nt.getLocalBounds();
        nt.setOrigin({b.position.x, b.position.y + b.size.y * 0.5f});
        nt.setPosition({formX - lineW * 0.46f, rowY});
        rw.draw(nt);
    }
    rowY += rowStep;

    drawMenuText(rw, am, "Color", {formX, rowY}, lSz, false, true);
    rowY += rowStep * 0.5f;
    {
        const float swSz = H * 0.054f;
        const float swGap = swSz * 1.3f;
        float swStartX = formX - swGap * 1.5f;
        int selColor = (p < static_cast<int>(setup_.playerColors.size()))
                       ? setup_.playerColors[static_cast<size_t>(p)] : p;
        for (int ci = 0; ci < 4; ++ci) {
            sf::RectangleShape sw({swSz, swSz});
            sw.setOrigin({swSz * 0.5f, swSz * 0.5f});
            sw.setPosition({swStartX + ci * swGap, rowY});
            sw.setFillColor(tabColors[ci]);
            sw.setOutlineThickness(ci == selColor ? 3.f : 1.f);
            sw.setOutlineColor(ci == selColor ? sf::Color(255,255,255)
                                              : sf::Color(255,255,255,60));
            rw.draw(sw);
        }
    }
    rowY += rowStep;

    drawMenuText(rw, am, "Type", {formX, rowY}, lSz, false, true);
    rowY += rowStep * 0.5f;
    {
        bool isComp = (p < static_cast<int>(setup_.isComputer.size()))
                      && setup_.isComputer[static_cast<size_t>(p)];
        unsigned typeSz = static_cast<unsigned>(H * 0.034f);
        drawMenuText(rw, am, "Human",    {formX - H * 0.09f, rowY}, typeSz, false, !isComp);
        drawMenuText(rw, am, "Computer", {formX + H * 0.09f, rowY}, typeSz, false,  isComp);
    }

    unsigned nextSz = static_cast<unsigned>(H * 0.034f);
    drawMenuText(rw, am, "Next >", {cx + W * 0.22f, cy + panelH * 0.44f},
                 nextSz, hoveredItem_ == 10);

    rw.display();
#endif
}

void GUIView::drawCustMap() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0, 0, 0));
    drawMenuBackground(rw, am);

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned hSz = static_cast<unsigned>(H * 0.042f);
    drawMenuText(rw, am, "< Customize Map", {cx, H * 0.10f}, hSz, false, true);

    unsigned lblSz = static_cast<unsigned>(H * 0.03f);
    drawMenuText(rw, am, "Number of Tiles", {cx, cy - H * 0.22f}, lblSz, false, true);

    const int tileOptions[] = {20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
    const int N = 11;
    const int cols = 4;
    const float btnW = H * 0.12f, btnH = H * 0.058f, gap = H * 0.015f;
    const float gridW = cols * (btnW + gap) - gap;
    const float startX = cx - gridW * 0.5f + btnW * 0.5f;
    const float startY = cy - H * 0.12f;

    for (int i = 0; i < N; ++i) {
        int col = i % cols, row = i / cols;
        float bx = startX + col * (btnW + gap);
        float by = startY + row * (btnH + gap);
        bool sel = (setup_.numTiles == tileOptions[i]);
        unsigned optSz = static_cast<unsigned>(btnH * 0.9f);
        drawMenuText(rw, am, std::to_string(tileOptions[i]),
                 {bx, by}, optSz, hoveredItem_ == i, sel);
    }

        unsigned startSz = static_cast<unsigned>(H * 0.038f);
        drawMenuText(rw, am, "Start Game",
             {cx, cy + H * 0.32f}, startSz, hoveredItem_ == 20);

    rw.display();
#endif
}

void GUIView::drawLoadGame() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0, 0, 0));
    drawMenuBackground(rw, am);

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned hSz = static_cast<unsigned>(H * 0.042f);
    drawMenuText(rw, am, "< Back", {cx, cy - H * 0.30f}, hSz, hoveredItem_ == 20, false);
    drawMenuText(rw, am, "Load Game", {cx, cy - H * 0.20f}, hSz, false, true);

    unsigned lblSz = static_cast<unsigned>(H * 0.03f);
    drawMenuText(rw, am, "Enter save file path:", {cx, cy - H * 0.10f}, lblSz, false, true);

    const float boxW = H * 0.54f;
    sf::RectangleShape line({boxW, 2.f});
    line.setPosition({cx - boxW * 0.5f, cy + H * 0.02f});
    line.setFillColor(sf::Color(220, 230, 240, 170));
    rw.draw(line);

    std::string display = setup_.loadFilePath.empty() ? "saves/game.json" : setup_.loadFilePath;
    sf::Color textC = setup_.loadFilePath.empty() ? sf::Color(100, 120, 150) : sf::Color(230, 240, 255);
    unsigned tsz = static_cast<unsigned>(H * 0.026f);
    sf::Text t(am.font("title"), display, tsz);
    t.setFillColor(textC);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x, b.position.y + b.size.y * 0.5f});
    t.setPosition({cx - boxW * 0.46f, cy});
    rw.draw(t);

    unsigned loadSz = static_cast<unsigned>(H * 0.036f);
    drawMenuText(rw, am, "Load", {cx, cy + H * 0.14f}, loadSz, hoveredItem_ == 0);

    rw.display();
#endif
}

void GUIView::drawGameOver() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0x1a, 0x27, 0x44));
    const sf::Texture* globeTex = am.texture("assets/bg/Globe.png");
    if (globeTex) {
        gui::draw::drawSpriteCover(rw, globeTex);
    } else {
        gui::draw::drawGlobeBackground(rw);
    }

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned titleSz = static_cast<unsigned>(H * 0.08f);
    sf::Text title(am.font("title"), "GAME OVER", titleSz);
    title.setFillColor(sf::Color(235, 240, 248));
    auto tb = title.getLocalBounds();
    title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
    title.setPosition({cx, cy - H * 0.22f});
    rw.draw(title);

    // Winners
    float y = cy - H * 0.08f;
    if (!winnerInfo_.winners.empty()) {
        std::string winText = "Winner: " + winnerInfo_.winners.front();
        unsigned winSz = static_cast<unsigned>(H * 0.045f);
        sf::Text wt(am.font("bold"), winText, winSz);
        wt.setFillColor(sf::Color(0xff, 0xf2, 0x00));
        auto wb = wt.getLocalBounds();
        wt.setOrigin({wb.position.x + wb.size.x * 0.5f, wb.position.y + wb.size.y * 0.5f});
        wt.setPosition({cx, y});
        rw.draw(wt);
        y += H * 0.07f;
    }

    // Ranking
    unsigned rankSz = static_cast<unsigned>(H * 0.028f);
    for (size_t i = 0; i < winnerInfo_.players.size(); ++i) {
        const auto& ps = winnerInfo_.players[i];
        std::string line = std::to_string(i + 1) + ". " + ps.username +
                           "  —  " + ps.money.toString();
        sf::Text rt(am.font("regular"), line, rankSz);
        rt.setFillColor(i == 0 ? sf::Color(0xff, 0xf2, 0x00) : sf::Color(210, 220, 235));
        auto rb = rt.getLocalBounds();
        rt.setOrigin({rb.position.x + rb.size.x * 0.5f, rb.position.y + rb.size.y * 0.5f});
        rt.setPosition({cx, y + static_cast<float>(i) * rankSz * 1.5f});
        rw.draw(rt);
    }

    // Back to menu hint
    unsigned hintSz = static_cast<unsigned>(H * 0.022f);
    sf::Text hint(am.font("regular"), "Press ESC or click to return to menu", hintSz);
    hint.setFillColor(sf::Color(150, 160, 180));
    auto hb = hint.getLocalBounds();
    hint.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y + hb.size.y * 0.5f});
    hint.setPosition({cx, H * 0.92f});
    rw.draw(hint);

    rw.display();
#endif
}

bool GUIView::handleMenuEvent(const sf::Event& event) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return false;
    const float H = static_cast<float>(window->getSize().y);
    const float cx = static_cast<float>(window->getSize().x) * 0.5f;
    const float cy = static_cast<float>(window->getSize().y) * 0.5f;

    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        float mx = static_cast<float>(mm->position.x);
        float my = static_cast<float>(mm->position.y);
        hoveredItem_ = -1;

        if (screen_ == AppScreen::LANDING) {
            const float btnW = H * 0.28f, btnH = H * 0.058f;
            const float startY = cy + H * 0.22f, gap = H * 0.072f;
            for (int i = 0; i < 4; ++i) {
                float by = startY + i * gap;
                if (mx >= cx - btnW * 0.5f && mx <= cx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    hoveredItem_ = i;
            }
        } else if (screen_ == AppScreen::NEW_GAME_NUM_PLAYERS) {
            const float panelH = H * 0.42f;
            const float btnW = H * 0.55f * 0.55f, btnH = H * 0.066f;
            const float backY = cy - panelH * 0.48f;
            const float backW = H * 0.18f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - btnH * 0.5f && my <= backY + btnH * 0.5f)
                hoveredItem_ = 20;
            const float startY = cy - panelH * 0.08f, gap = H * 0.09f;
            for (int i = 0; i < 3; ++i) {
                float by = startY + i * gap;
                if (mx >= cx - btnW * 0.5f && mx <= cx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    hoveredItem_ = i;
            }
            float nextY = cy + panelH * 0.58f;
            if (my >= nextY - btnH * 0.5f && my <= nextY + btnH * 0.5f)
                hoveredItem_ = 10;
        } else if (screen_ == AppScreen::NEW_GAME_CUST_PLAYER) {
            const float backY = H * 0.10f;
            const float backW = H * 0.28f, backH = H * 0.058f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - backH * 0.5f && my <= backY + backH * 0.5f)
                hoveredItem_ = 20;
        } else if (screen_ == AppScreen::NEW_GAME_CUST_MAP) {
            const int tileOptions[] = {20,24,28,32,36,40,44,48,52,56,60};
            const int cols = 4;
            const float btnW = H * 0.12f, btnH = H * 0.058f, gap = H * 0.015f;
            const float gridW = cols * (btnW + gap) - gap;
            const float startX = cx - gridW * 0.5f + btnW * 0.5f;
            const float startY = cy - H * 0.12f;
            for (int i = 0; i < 11; ++i) {
                float bx = startX + (i % cols) * (btnW + gap);
                float by = startY + (i / cols) * (btnH + gap);
                if (mx >= bx - btnW * 0.5f && mx <= bx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    hoveredItem_ = i;
                (void)tileOptions;
            }
            float startY2 = cy + H * 0.32f;
            const float bigW = H * 0.24f, bigH = H * 0.062f;
            if (mx >= cx - bigW * 0.5f && mx <= cx + bigW * 0.5f &&
                my >= startY2 - bigH * 0.5f && my <= startY2 + bigH * 0.5f)
                hoveredItem_ = 20;
        } else if (screen_ == AppScreen::LOAD_GAME) {
            float backY = cy - H * 0.30f;
            const float backW = H * 0.18f, backH = H * 0.062f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - backH * 0.5f && my <= backY + backH * 0.5f)
                hoveredItem_ = 20;
            float loadY = cy + H * 0.14f;
            const float bW = H * 0.18f, bH = H * 0.062f;
            if (mx >= cx - bW * 0.5f && mx <= cx + bW * 0.5f &&
                my >= loadY - bH * 0.5f && my <= loadY + bH * 0.5f)
                hoveredItem_ = 0;
        }
        return false;
    }

    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button != sf::Mouse::Button::Left) return false;
        float mx = static_cast<float>(mb->position.x);
        float my = static_cast<float>(mb->position.y);

        if (screen_ == AppScreen::LANDING) {
            const float btnW = H * 0.28f, btnH = H * 0.058f;
            const float startY = cy + H * 0.22f, gap = H * 0.072f;
            for (int i = 0; i < 4; ++i) {
                float by = startY + i * gap;
                if (mx >= cx - btnW * 0.5f && mx <= cx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f) {
                    if (i == 0) { screen_ = AppScreen::NEW_GAME_NUM_PLAYERS; hoveredItem_ = -1; }
                    else if (i == 1) { screen_ = AppScreen::LOAD_GAME; hoveredItem_ = -1; }
                    else if (i == 3) { window->close(); }
                }
            }
        } else if (screen_ == AppScreen::NEW_GAME_NUM_PLAYERS) {
            const float panelH = H * 0.42f;
            const float btnW = H * 0.55f * 0.55f, btnH = H * 0.066f;
            float backY = cy - panelH * 0.48f;
            float backW = H * 0.18f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - btnH * 0.5f && my <= backY + btnH * 0.5f) {
                screen_ = AppScreen::LANDING;
                hoveredItem_ = -1;
            }
            const float startY = cy - panelH * 0.08f, gap = H * 0.09f;
            for (int i = 0; i < 3; ++i) {
                float by = startY + i * gap;
                if (mx >= cx - btnW * 0.5f && mx <= cx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    setup_.numPlayers = i + 2;
            }
            float nextY = cy + panelH * 0.58f;
            const float nextW = btnW * 0.5f;
            if (mx >= cx - nextW * 0.5f && mx <= cx + nextW * 0.5f &&
                my >= nextY - btnH * 0.5f && my <= nextY + btnH * 0.5f) {
                setup_.playerNames.assign(static_cast<size_t>(setup_.numPlayers), "");
                setup_.isComputer.assign(static_cast<size_t>(setup_.numPlayers), false);
                setup_.playerColors.clear();
                for (int i = 0; i < setup_.numPlayers; ++i)
                    setup_.playerColors.push_back(i);
                custPlayerTab_ = 0;
                screen_ = AppScreen::NEW_GAME_CUST_PLAYER;
                hoveredItem_ = -1;
            }
        } else if (screen_ == AppScreen::NEW_GAME_CUST_PLAYER) {
            float backY = H * 0.10f;
            float backW = H * 0.28f, backH = H * 0.058f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - backH * 0.5f && my <= backY + backH * 0.5f) {
                screen_ = AppScreen::NEW_GAME_NUM_PLAYERS;
                hoveredItem_ = -1;
            }
            const float H2 = H;
            const float tabW = static_cast<float>(window->getSize().x) * 0.12f;
            const float tabH = H2 * 0.07f;
            const float tabX = static_cast<float>(window->getSize().x) * 0.18f;
            const float tabStartY = cy - setup_.numPlayers * tabH * 0.5f;
            for (int i = 0; i < setup_.numPlayers; ++i) {
                float ty = tabStartY + i * tabH;
                if (mx >= tabX - tabW * 0.5f && mx <= tabX + tabW * 0.5f &&
                    my >= ty - tabH * 0.85f * 0.5f && my <= ty + tabH * 0.85f * 0.5f)
                    custPlayerTab_ = i;
            }
            int p = custPlayerTab_;
            float formX = cx - static_cast<float>(window->getSize().x) * 0.02f;
            float typeY = cy - H * 0.42f * 0.35f + H * 0.1f * 2.5f;
            const float tbH = H * 0.054f;
            if (my >= typeY - tbH * 0.5f && my <= typeY + tbH * 0.5f) {
                if (mx >= formX - H * 0.18f && mx <= formX - H * 0.02f) {
                    if (p < static_cast<int>(setup_.isComputer.size()))
                        setup_.isComputer[static_cast<size_t>(p)] = false;
                } else if (mx >= formX + H * 0.01f && mx <= formX + H * 0.18f) {
                    if (p < static_cast<int>(setup_.isComputer.size()))
                        setup_.isComputer[static_cast<size_t>(p)] = true;
                }
            }
            float clrY = cy - H * 0.42f * 0.35f + H * 0.1f * 1.5f;
            const float swSz = H * 0.054f, swGap = swSz * 1.3f;
            float swStartX = formX - swGap * 1.5f;
            for (int ci = 0; ci < 4; ++ci) {
                float sx = swStartX + ci * swGap;
                if (mx >= sx - swSz * 0.5f && mx <= sx + swSz * 0.5f &&
                    my >= clrY - swSz * 0.5f && my <= clrY + swSz * 0.5f) {
                    if (p < static_cast<int>(setup_.playerColors.size()))
                        setup_.playerColors[static_cast<size_t>(p)] = ci;
                }
            }
            float nextX = cx + static_cast<float>(window->getSize().x) * 0.22f;
            float nextY = cy + H * 0.60f * 0.44f;
            const float nW = H * 0.16f, nH = H * 0.054f;
            if (mx >= nextX - nW * 0.5f && mx <= nextX + nW * 0.5f &&
                my >= nextY - nH * 0.5f && my <= nextY + nH * 0.5f) {
                screen_ = AppScreen::NEW_GAME_CUST_MAP;
                hoveredItem_ = -1;
            }
        } else if (screen_ == AppScreen::NEW_GAME_CUST_MAP) {
            float backY = H * 0.10f;
            float backW = H * 0.28f, backH = H * 0.058f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - backH * 0.5f && my <= backY + backH * 0.5f) {
                screen_ = AppScreen::NEW_GAME_CUST_PLAYER;
                hoveredItem_ = -1;
            }
            const int tileOptions[] = {20,24,28,32,36,40,44,48,52,56,60};
            const int cols = 4;
            const float btnW = H * 0.12f, btnH = H * 0.058f, gap = H * 0.015f;
            const float gridW = cols * (btnW + gap) - gap;
            const float startX = cx - gridW * 0.5f + btnW * 0.5f;
            const float startY = cy - H * 0.12f;
            for (int i = 0; i < 11; ++i) {
                float bx = startX + (i % cols) * (btnW + gap);
                float by = startY + (i / cols) * (btnH + gap);
                if (mx >= bx - btnW * 0.5f && mx <= bx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    setup_.numTiles = tileOptions[i];
            }
            float startBtnY = cy + H * 0.32f;
            const float bigW = H * 0.24f, bigH = H * 0.062f;
            if (mx >= cx - bigW * 0.5f && mx <= cx + bigW * 0.5f &&
                my >= startBtnY - bigH * 0.5f && my <= startBtnY + bigH * 0.5f) {
                screen_ = AppScreen::IN_GAME;
                return true;   // signal: start game
            }
        } else if (screen_ == AppScreen::LOAD_GAME) {
            float backY = cy - H * 0.30f;
            float backW = H * 0.18f, backH = H * 0.062f;
            if (mx >= cx - backW * 0.5f && mx <= cx + backW * 0.5f &&
                my >= backY - backH * 0.5f && my <= backY + backH * 0.5f) {
                screen_ = AppScreen::LANDING;
                hoveredItem_ = -1;
            }
            float loadY = cy + H * 0.14f;
            const float bW = H * 0.18f, bH = H * 0.062f;
            if (mx >= cx - bW * 0.5f && mx <= cx + bW * 0.5f &&
                my >= loadY - bH * 0.5f && my <= loadY + bH * 0.5f) {
                screen_ = AppScreen::IN_GAME;
                return true;   // signal: load game
            }
        } else if (screen_ == AppScreen::GAME_OVER) {
            screen_ = AppScreen::LANDING;
            hoveredItem_ = -1;
            return false;
        }
        return false;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            if (screen_ == AppScreen::GAME_OVER) {
                screen_ = AppScreen::LANDING;
            } else if (screen_ == AppScreen::NEW_GAME_NUM_PLAYERS ||
                screen_ == AppScreen::LOAD_GAME)
                screen_ = AppScreen::LANDING;
            else if (screen_ == AppScreen::NEW_GAME_CUST_PLAYER)
                screen_ = AppScreen::NEW_GAME_NUM_PLAYERS;
            else if (screen_ == AppScreen::NEW_GAME_CUST_MAP)
                screen_ = AppScreen::NEW_GAME_CUST_PLAYER;
            hoveredItem_ = -1;
        }
        if (screen_ == AppScreen::LOAD_GAME) {
            if (kp->code == sf::Keyboard::Key::Backspace && !setup_.loadFilePath.empty())
                setup_.loadFilePath.pop_back();
        }
    }
    if (const auto* ti = event.getIf<sf::Event::TextEntered>()) {
        if (screen_ == AppScreen::LOAD_GAME && ti->unicode >= 32 && ti->unicode < 127) {
            setup_.loadFilePath += static_cast<char>(ti->unicode);
        }
        if (screen_ == AppScreen::NEW_GAME_CUST_PLAYER) {
            int p = custPlayerTab_;
            while (static_cast<int>(setup_.playerNames.size()) <= p)
                setup_.playerNames.emplace_back();
            if (ti->unicode == 8) {  // backspace
                if (!setup_.playerNames[static_cast<size_t>(p)].empty())
                    setup_.playerNames[static_cast<size_t>(p)].pop_back();
            } else if (ti->unicode >= 32 && ti->unicode < 127 &&
                       setup_.playerNames[static_cast<size_t>(p)].size() < 16) {
                setup_.playerNames[static_cast<size_t>(p)] += static_cast<char>(ti->unicode);
            }
        }
    }
#else
    (void)event;
#endif
    return false;
}
