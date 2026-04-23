#include "ui/GUIView.hpp"

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <array>
#include <string>
#endif

GUIView::GUIView(sf::RenderWindow& window) : window(&window) {}

#if NIMONSPOLY_ENABLE_SFML

static sf::Color tileColor(Color c) {
    switch (c) {
        case Color::BROWN:      return sf::Color(0xda, 0xcc, 0xc1);
        case Color::LIGHT_BLUE: return sf::Color(0x2e, 0xc7, 0xff);
        case Color::PINK:       return sf::Color(0xee, 0x2a, 0x89);
        case Color::ORANGE:     return sf::Color(0xff, 0x77, 0x00);
        case Color::RED:        return sf::Color(0xbe, 0x17, 0x1a);
        case Color::YELLOW:     return sf::Color(0xff, 0xf2, 0x00);
        case Color::GREEN:      return sf::Color(0x00, 0x93, 0x64);
        case Color::DARK_BLUE:  return sf::Color(0x12, 0x48, 0x72);
        case Color::GRAY:       return sf::Color(0xa0, 0xa0, 0xa0);
        default:                return sf::Color(0x37, 0x3c, 0x46);
    }
}

static sf::Color tokenColor(int playerIndex) {
    constexpr std::array<sf::Color, 4> colors{
        sf::Color(0x00, 0xc8, 0xff),
        sf::Color(0xff, 0x2d, 0x8a),
        sf::Color(0xff, 0xf2, 0x00),
        sf::Color(0x00, 0xff, 0xb0),
    };
    return colors[static_cast<size_t>(playerIndex) % 4];
}

static void drawTileCard(sf::RenderWindow& rw,
                          sf::Vector2f pos, float tileSz,
                          const TileView& tv,
                          AssetManager& am) {
    const int side = tv.index / 10;
    float rotDeg = 0.f;
    if      (side == 0) rotDeg = 180.f;
    else if (side == 1) rotDeg =  90.f;
    else if (side == 3) rotDeg = 270.f;

    sf::Vector2f center{pos.x + tileSz * 0.5f, pos.y + tileSz * 0.5f};
    sf::Transform tfm;
    tfm.rotate(sf::degrees(rotDeg), center);
    sf::RenderStates rs(tfm);

    const sf::Color grpColor = tileColor(tv.color);
    const bool hasGroup = (tv.color != Color::DEFAULT);

    sf::RectangleShape bg({tileSz, tileSz});
    bg.setPosition(pos);
    bg.setFillColor(hasGroup ? sf::Color(250, 250, 248) : sf::Color(50, 54, 66));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(18, 20, 25));
    rw.draw(bg, rs);

    const float photoFrac = 0.62f;
    const float photoH    = tileSz * photoFrac;
    const sf::Texture* tex = am.tileTexture(tv.code);

    if (tex) {
        auto texSz = tex->getSize();
        float scaleX = tileSz / static_cast<float>(texSz.x);
        float scaleY = photoH / static_cast<float>(texSz.y);
        float scale  = std::min(scaleX, scaleY);
        float sw = static_cast<float>(texSz.x) * scale;
        float sh = static_cast<float>(texSz.y) * scale;
        sf::Sprite sprite(*tex);
        sprite.setScale({scale, scale});
        sprite.setPosition({pos.x + (tileSz - sw) * 0.5f,
                            pos.y + (photoH - sh) * 0.5f});
        rw.draw(sprite, rs);
    } else if (hasGroup) {
        sf::RectangleShape fill({tileSz, photoH});
        fill.setPosition(pos);
        fill.setFillColor(sf::Color(grpColor.r, grpColor.g, grpColor.b, 100));
        rw.draw(fill, rs);
    }

    if (hasGroup) {
        sf::RectangleShape strip({tileSz, tileSz * 0.08f});
        strip.setPosition({pos.x, pos.y + photoH});
        strip.setFillColor(grpColor);
        rw.draw(strip, rs);
    }

    const float badgeFrac = 0.22f;
    const float badgeY    = pos.y + tileSz * (1.f - badgeFrac);
    sf::RectangleShape badge({tileSz, tileSz * badgeFrac});
    badge.setPosition({pos.x, badgeY});
    badge.setFillColor(sf::Color(255, 255, 255, 210));
    rw.draw(badge, rs);

    const sf::Font& font = am.font("bold");
    unsigned charSz = static_cast<unsigned>(tileSz * 0.17f);
    if (charSz < 6u) charSz = 6u;
    sf::Text label(font, tv.code, charSz);
    sf::Color textColor = hasGroup ? grpColor : sf::Color(200, 200, 200);
    // Yellow is hard to read on white badge — darken it
    if (tv.color == Color::YELLOW)
        textColor = sf::Color(0x90, 0x87, 0x00);
    label.setFillColor(textColor);
    auto lb = label.getLocalBounds();
    label.setOrigin({lb.position.x + lb.size.x * 0.5f,
                     lb.position.y + lb.size.y * 0.5f});
    label.setPosition({pos.x + tileSz * 0.5f,
                       badgeY + tileSz * badgeFrac * 0.5f});
    rw.draw(label, rs);
}

static void drawGlobeBackground(sf::RenderWindow& rw) {
    const sf::Vector2u sz = rw.getSize();
    const float cx = sz.x * 0.5f;
    const float cy = sz.y * 0.5f;
    const float r  = std::min(sz.x, sz.y) * 0.44f;

    sf::CircleShape glow(r * 1.35f);
    glow.setFillColor(sf::Color(100, 160, 255, 18));
    glow.setOrigin({r * 1.35f, r * 1.35f});
    glow.setPosition({cx, cy});
    rw.draw(glow);

    sf::CircleShape globe(r);
    globe.setFillColor(sf::Color(0x0d, 0x18, 0x2e));
    globe.setOutlineThickness(2.f);
    globe.setOutlineColor(sf::Color(100, 160, 255, 80));
    globe.setOrigin({r, r});
    globe.setPosition({cx, cy});
    rw.draw(globe);

    sf::CircleShape atm(r * 0.92f);
    atm.setFillColor(sf::Color(0, 0, 0, 0));
    atm.setOutlineThickness(r * 0.06f);
    atm.setOutlineColor(sf::Color(120, 180, 255, 40));
    atm.setOrigin({r * 0.92f, r * 0.92f});
    atm.setPosition({cx, cy});
    rw.draw(atm);
}

static void drawCenteredText(sf::RenderWindow& rw, AssetManager& am,
                              const std::string& fontKey, const std::string& str,
                              unsigned charSz, sf::Color color, float y) {
    sf::Text t(am.font(fontKey), str, charSz);
    t.setFillColor(color);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    t.setPosition({static_cast<float>(rw.getSize().x) * 0.5f, y});
    rw.draw(t);
}

static void drawMenuButton(sf::RenderWindow& rw, AssetManager& am,
                           const std::string& label,
                           sf::Vector2f center, sf::Vector2f sz,
                           bool hovered, bool selected = false) {
    sf::RectangleShape btn(sz);
    btn.setOrigin({sz.x * 0.5f, sz.y * 0.5f});
    btn.setPosition(center);
    sf::Color fill = selected  ? sf::Color(255, 255, 255, 210)
                   : hovered   ? sf::Color(255, 255, 255,  60)
                               : sf::Color(255, 255, 255,  20);
    btn.setFillColor(fill);
    btn.setOutlineThickness(1.f);
    btn.setOutlineColor(sf::Color(255, 255, 255, selected ? 200 : 60));
    rw.draw(btn);

    sf::Color tc = (selected || hovered) ? sf::Color(255, 255, 255)
                                         : sf::Color(180, 200, 220);
    unsigned csz = static_cast<unsigned>(sz.y * 0.52f);
    sf::Text t(am.font("bold"), label, csz);
    t.setFillColor(tc);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    t.setPosition(center);
    rw.draw(t);
}

static void drawSprite(sf::RenderWindow& rw, const sf::Texture* tex,
                       sf::FloatRect dest,
                       sf::RenderStates rs = sf::RenderStates::Default) {
    if (!tex) return;
    auto tsz = tex->getSize();
    sf::Sprite sprite(*tex);
    sprite.setScale({dest.size.x / static_cast<float>(tsz.x),
                     dest.size.y / static_cast<float>(tsz.y)});
    sprite.setPosition({dest.position.x, dest.position.y});
    rw.draw(sprite, rs);
}
#endif  // NIMONSPOLY_ENABLE_SFML

void GUIView::drawLandingPage() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0x1a, 0x27, 0x44));

    const float W  = static_cast<float>(rw.getSize().x);
    const float H  = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;

    const sf::Texture* globeTex = am.texture("assets/bg/Globe.png");
    float globeSz = H * 0.60f;
    if (globeTex) {
        drawSprite(rw, globeTex,
                   {{cx - globeSz * 0.5f, cy - globeSz * 0.55f},
                    {globeSz, globeSz}});
    } else {
        drawGlobeBackground(rw);
    }

    const sf::Texture* titleTex = am.texture("assets/bg/Title.png");
    if (titleTex) {
        auto tsz = titleTex->getSize();
        float titleW = H * 0.48f;
        float titleH = titleW * static_cast<float>(tsz.y) / static_cast<float>(tsz.x);
        sf::RenderStates addBlend;
        addBlend.blendMode = sf::BlendAdd;
        drawSprite(rw, titleTex,
                   {{cx - titleW * 0.5f, cy - H * 0.04f - titleH * 0.5f},
                    {titleW, titleH}},
                   addBlend);
    } else {
        unsigned titleSz = static_cast<unsigned>(H * 0.12f);
        drawCenteredText(rw, am, "title", "NIMONS", titleSz,
                         sf::Color(255, 255, 255), cy - H * 0.10f);
        drawCenteredText(rw, am, "title", "POLY",   titleSz,
                         sf::Color(255, 255, 255), cy + H * 0.02f);
        unsigned suitSz = static_cast<unsigned>(H * 0.08f);
        sf::Text ls(am.font("bold"), "♠", suitSz);
        ls.setFillColor(sf::Color(255, 255, 255, 80));
        auto b = ls.getLocalBounds();
        ls.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        ls.setPosition({cx - H * 0.32f, cy - H * 0.04f});
        rw.draw(ls);
        sf::Text rs2(am.font("bold"), "♣", suitSz);
        rs2.setFillColor(sf::Color(255, 255, 255, 80));
        b = rs2.getLocalBounds();
        rs2.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        rs2.setPosition({cx + H * 0.32f, cy - H * 0.04f});
        rw.draw(rs2);
    }

    unsigned subSz = static_cast<unsigned>(H * 0.025f);
    drawCenteredText(rw, am, "regular", "presented by BurntCheesecake", subSz,
                     sf::Color(140, 170, 200), cy + H * 0.12f);

    const char* items[] = {"New Game", "Load Game", "Credits", "Exit"};
    const float btnW = H * 0.28f;
    const float btnH = H * 0.058f;
    const float startY = cy + H * 0.22f;
    const float gap    = H * 0.072f;
    const sf::Texture* btnTex = am.texture("assets/components/btn/EmptyRectBtn.png");
    for (int i = 0; i < 4; ++i) {
        float bx  = cx - btnW * 0.5f;
        float by  = startY + i * gap - btnH * 0.5f;
        bool  hov = hoveredItem_ == i;
        if (btnTex) {
            auto bsz = btnTex->getSize();
            sf::Sprite btnSpr(*btnTex);
            btnSpr.setScale({btnW / static_cast<float>(bsz.x),
                             btnH / static_cast<float>(bsz.y)});
            btnSpr.setPosition({bx, by});
            btnSpr.setColor(hov ? sf::Color(255, 255, 255, 240)
                                : sf::Color(200, 210, 230, 210));
            rw.draw(btnSpr);
        } else {
            drawMenuButton(rw, am, "", {cx, startY + i * gap}, {btnW, btnH}, hov);
        }
        sf::Color tc = hov ? sf::Color(15, 25, 50) : sf::Color(40, 60, 100);
        unsigned csz = static_cast<unsigned>(btnH * 0.50f);
        sf::Text t(am.font("bold"), items[i], csz);
        t.setFillColor(tc);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({cx, startY + i * gap});
        rw.draw(t);
    }

    rw.display();
#endif
}

void GUIView::drawNumPlayers() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0x1a, 0x27, 0x44));
    drawGlobeBackground(rw);

    const float cx = static_cast<float>(rw.getSize().x) * 0.5f;
    const float cy = static_cast<float>(rw.getSize().y) * 0.5f;
    const float h  = static_cast<float>(rw.getSize().y);

    const float panelW = h * 0.55f, panelH = h * 0.42f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setOrigin({panelW * 0.5f, panelH * 0.5f});
    panel.setPosition({cx, cy});
    panel.setFillColor(sf::Color(20, 32, 60, 220));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(100, 140, 200, 80));
    rw.draw(panel);

    unsigned hSz = static_cast<unsigned>(h * 0.038f);
    drawCenteredText(rw, am, "bold", "Choose number of players", hSz,
                     sf::Color(220, 235, 255), cy - panelH * 0.32f);

    const char* opts[] = {"2 Players", "3 Players", "4 Players"};
    const float btnW = panelW * 0.55f, btnH = h * 0.066f;
    const float startY = cy - panelH * 0.08f;
    const float gap    = h * 0.09f;
    for (int i = 0; i < 3; ++i) {
        bool sel = (setup_.numPlayers == i + 2);
        drawMenuButton(rw, am, opts[i],
                       {cx, startY + i * gap}, {btnW, btnH},
                       hoveredItem_ == i, sel);
    }

    drawMenuButton(rw, am, "Next >",
                   {cx, cy + panelH * 0.38f}, {btnW * 0.5f, btnH},
                   hoveredItem_ == 10);

    rw.display();
#endif
}

void GUIView::drawCustPlayer() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0x1a, 0x27, 0x44));
    drawGlobeBackground(rw);

    const float W  = static_cast<float>(rw.getSize().x);
    const float H  = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned hSz = static_cast<unsigned>(H * 0.036f);
    drawCenteredText(rw, am, "bold", "< Customize Player", hSz,
                     sf::Color(220, 235, 255), H * 0.08f);

    const float tabW = W * 0.12f, tabH = H * 0.07f;
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
        sf::RectangleShape tab({tabW, tabH * 0.85f});
        tab.setOrigin({tabW * 0.5f, tabH * 0.85f * 0.5f});
        tab.setPosition({tabX, tabStartY + i * tabH});
        tab.setFillColor(sel ? sf::Color(tabColors[i % 4].r,
                                         tabColors[i % 4].g,
                                         tabColors[i % 4].b, 200)
                             : sf::Color(255, 255, 255, 25));
        tab.setOutlineThickness(1.f);
        tab.setOutlineColor(sf::Color(tabColors[i % 4].r,
                                      tabColors[i % 4].g,
                                      tabColors[i % 4].b, 120));
        rw.draw(tab);

        std::string label = "P" + std::to_string(i + 1);
        unsigned tsz = static_cast<unsigned>(tabH * 0.4f);
        sf::Text t(am.font("bold"), label, tsz);
        t.setFillColor(sel ? sf::Color(0, 0, 0) : sf::Color(180, 200, 220));
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({tabX, tabStartY + i * tabH});
        rw.draw(t);
    }

    const float panelX = cx - W * 0.02f;
    const float panelW = W * 0.42f, panelH = H * 0.60f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setOrigin({panelW * 0.5f, panelH * 0.5f});
    panel.setPosition({panelX, cy});
    panel.setFillColor(sf::Color(20, 32, 60, 220));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(100, 140, 200, 60));
    rw.draw(panel);

    int p = custPlayerTab_;
    float formX = panelX;
    float rowY = cy - panelH * 0.35f;
    float rowStep = H * 0.1f;
    unsigned lSz = static_cast<unsigned>(H * 0.028f);

    drawCenteredText(rw, am, "bold", "Name", lSz, sf::Color(160, 185, 215), rowY);
    rowY += rowStep * 0.5f;
    {
        sf::RectangleShape nameBox({panelW * 0.75f, H * 0.054f});
        nameBox.setOrigin({nameBox.getSize().x * 0.5f, nameBox.getSize().y * 0.5f});
        nameBox.setPosition({formX, rowY});
        nameBox.setFillColor(sf::Color(255, 255, 255, 30));
        nameBox.setOutlineThickness(1.f);
        nameBox.setOutlineColor(sf::Color(255, 255, 255, 80));
        rw.draw(nameBox);

        std::string name = (p < static_cast<int>(setup_.playerNames.size()))
                           ? setup_.playerNames[static_cast<size_t>(p)]
                           : "";
        sf::Text nt(am.font("bold"), name.empty() ? "Player " + std::to_string(p+1) : name,
                    static_cast<unsigned>(H * 0.026f));
        nt.setFillColor(name.empty() ? sf::Color(120, 140, 160) : sf::Color(230, 240, 255));
        auto b = nt.getLocalBounds();
        nt.setOrigin({b.position.x, b.position.y + b.size.y * 0.5f});
        nt.setPosition({formX - panelW * 0.75f * 0.46f, rowY});
        rw.draw(nt);
    }
    rowY += rowStep;

    drawCenteredText(rw, am, "bold", "Color", lSz, sf::Color(160, 185, 215), rowY);
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

    drawCenteredText(rw, am, "bold", "Type", lSz, sf::Color(160, 185, 215), rowY);
    rowY += rowStep * 0.5f;
    {
        bool isComp = (p < static_cast<int>(setup_.isComputer.size()))
                      && setup_.isComputer[static_cast<size_t>(p)];
        drawMenuButton(rw, am, "Human",    {formX - H * 0.09f, rowY},
                       {H * 0.14f, H * 0.054f}, false, !isComp);
        drawMenuButton(rw, am, "Computer", {formX + H * 0.09f, rowY},
                       {H * 0.16f, H * 0.054f}, false,  isComp);
    }

    drawMenuButton(rw, am, "Next >",
                   {cx + W * 0.22f, cy + panelH * 0.44f},
                   {H * 0.16f, H * 0.054f}, hoveredItem_ == 10);

    rw.display();
#endif
}

void GUIView::drawCustMap() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0x1a, 0x27, 0x44));
    drawGlobeBackground(rw);

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned hSz = static_cast<unsigned>(H * 0.036f);
    drawCenteredText(rw, am, "bold", "< Customize Map", hSz,
                     sf::Color(220, 235, 255), H * 0.08f);

    drawCenteredText(rw, am, "regular", "Number of Tiles", static_cast<unsigned>(H * 0.028f),
                     sf::Color(160, 185, 215), cy - H * 0.22f);

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
        drawMenuButton(rw, am, std::to_string(tileOptions[i]),
                       {bx, by}, {btnW, btnH},
                       hoveredItem_ == i, sel);
    }

    drawMenuButton(rw, am, "Start Game",
                   {cx, cy + H * 0.32f}, {H * 0.24f, H * 0.062f},
                   hoveredItem_ == 20);

    rw.display();
#endif
}

void GUIView::drawLoadGame() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    rw.clear(sf::Color(0x1a, 0x27, 0x44));
    drawGlobeBackground(rw);

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float cx = W * 0.5f, cy = H * 0.5f;

    unsigned hSz = static_cast<unsigned>(H * 0.038f);
    drawCenteredText(rw, am, "bold", "Load Game", hSz,
                     sf::Color(220, 235, 255), cy - H * 0.20f);

    drawCenteredText(rw, am, "regular", "Enter save file path:", static_cast<unsigned>(H * 0.026f),
                     sf::Color(160, 185, 215), cy - H * 0.10f);

    const float boxW = H * 0.54f, boxH = H * 0.058f;
    sf::RectangleShape box({boxW, boxH});
    box.setOrigin({boxW * 0.5f, boxH * 0.5f});
    box.setPosition({cx, cy});
    box.setFillColor(sf::Color(255, 255, 255, 30));
    box.setOutlineThickness(1.f);
    box.setOutlineColor(sf::Color(255, 255, 255, 100));
    rw.draw(box);

    std::string display = setup_.loadFilePath.empty() ? "saves/game.json" : setup_.loadFilePath;
    sf::Color textC = setup_.loadFilePath.empty() ? sf::Color(100, 120, 150) : sf::Color(230, 240, 255);
    unsigned tsz = static_cast<unsigned>(H * 0.026f);
    sf::Text t(am.font("regular"), display, tsz);
    t.setFillColor(textC);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x, b.position.y + b.size.y * 0.5f});
    t.setPosition({cx - boxW * 0.46f, cy});
    rw.draw(t);

    drawMenuButton(rw, am, "Load",
                   {cx, cy + H * 0.14f}, {H * 0.18f, H * 0.062f},
                   hoveredItem_ == 0);

    rw.display();
#endif
}

void GUIView::renderCurrentScreen() {
    switch (screen_) {
        case AppScreen::LANDING:              drawLandingPage(); break;
        case AppScreen::NEW_GAME_NUM_PLAYERS: drawNumPlayers();  break;
        case AppScreen::NEW_GAME_CUST_PLAYER: drawCustPlayer();  break;
        case AppScreen::NEW_GAME_CUST_MAP:    drawCustMap();     break;
        case AppScreen::LOAD_GAME:            drawLoadGame();    break;
        case AppScreen::IN_GAME:
        case AppScreen::GAME_OVER:            break;
    }
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
            const float startY = cy - panelH * 0.08f, gap = H * 0.09f;
            for (int i = 0; i < 3; ++i) {
                float by = startY + i * gap;
                if (mx >= cx - btnW * 0.5f && mx <= cx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    hoveredItem_ = i;
            }
            float nextY = cy + panelH * 0.38f;
            if (my >= nextY - btnH * 0.5f && my <= nextY + btnH * 0.5f)
                hoveredItem_ = 10;
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
            const float startY = cy - panelH * 0.08f, gap = H * 0.09f;
            for (int i = 0; i < 3; ++i) {
                float by = startY + i * gap;
                if (mx >= cx - btnW * 0.5f && mx <= cx + btnW * 0.5f &&
                    my >= by - btnH * 0.5f && my <= by + btnH * 0.5f)
                    setup_.numPlayers = i + 2;
            }
            float nextY = cy + panelH * 0.38f;
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
            float loadY = cy + H * 0.14f;
            const float bW = H * 0.18f, bH = H * 0.062f;
            if (mx >= cx - bW * 0.5f && mx <= cx + bW * 0.5f &&
                my >= loadY - bH * 0.5f && my <= loadY + bH * 0.5f) {
                screen_ = AppScreen::IN_GAME;
                return true;   // signal: load game
            }
        }
        return false;
    }

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            if (screen_ == AppScreen::NEW_GAME_NUM_PLAYERS ||
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

static constexpr float LP_W_FRAC  = 0.145f;
static constexpr float RP_W_FRAC  = 0.168f;
static constexpr float BOT_H_FRAC = 0.168f;

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

    const float boardSz = std::min(cW, cH) * 0.93f;
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

    rw.clear(sf::Color(0xc5, 0xce, 0xd8));

    sf::RectangleShape inner({innerSz, innerSz});
    inner.setPosition({origin.x + tileSz, origin.y + tileSz});
    inner.setFillColor(sf::Color(0xec, 0xf0, 0xf4));
    rw.draw(inner);

    {
        float innerX = origin.x + tileSz;
        float innerY = origin.y + tileSz;
        const sf::Texture* globeTex = am.texture("assets/bg/Globe.png");
        if (globeTex) {
            float globeSz = innerSz * 0.80f;
            drawSprite(rw, globeTex,
                       {{innerX + (innerSz - globeSz) * 0.5f,
                         innerY + (innerSz - globeSz) * 0.5f},
                        {globeSz, globeSz}});
        }
        const sf::Texture* titleTex = am.texture("assets/bg/Title.png");
        if (titleTex) {
            auto tsz2 = titleTex->getSize();
            float titleW = innerSz * 0.55f;
            float titleH = titleW * static_cast<float>(tsz2.y) / static_cast<float>(tsz2.x);
            sf::RenderStates addBlend;
            addBlend.blendMode = sf::BlendAdd;
            drawSprite(rw, titleTex,
                       {{innerX + (innerSz - titleW) * 0.5f,
                         innerY + (innerSz - titleH) * 0.5f},
                        {titleW, titleH}},
                       addBlend);
        } else {
            float logoSz = innerSz * 0.28f;
            unsigned charSz = static_cast<unsigned>(logoSz);
            sf::Text line1(am.font("title"), std::string("NIMONS"), charSz);
            sf::Text line2(am.font("title"), std::string("POLY"),   charSz);
            line1.setFillColor(sf::Color(50, 60, 80));
            line2.setFillColor(sf::Color(50, 60, 80));
            float icx = innerX + innerSz * 0.5f;
            float icy = innerY + innerSz * 0.5f;
            auto b1 = line1.getLocalBounds();
            auto b2 = line2.getLocalBounds();
            line1.setOrigin({b1.position.x + b1.size.x * 0.5f, b1.position.y + b1.size.y});
            line2.setOrigin({b2.position.x + b2.size.x * 0.5f, b2.position.y});
            line1.setPosition({icx, icy - logoSz * 0.08f});
            line2.setPosition({icx, icy + logoSz * 0.08f});
            rw.draw(line1);
            rw.draw(line2);
        }
    }

    const int numTiles = std::min<int>(TOTAL, static_cast<int>(state.tiles.size()));
    for (int i = 0; i < numTiles; ++i)
        drawTileCard(rw, tilePos(i), tileSz, state.tiles[static_cast<size_t>(i)], am);

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
            token.setFillColor(tokenColor(playerIdx));
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

#if NIMONSPOLY_ENABLE_SFML
static void drawLabel(sf::RenderWindow& rw, AssetManager& am,
                      const std::string& fontKey, const std::string& str,
                      unsigned sz, sf::Color col, sf::Vector2f pos) {
    sf::Text t(am.font(fontKey), str, sz);
    t.setFillColor(col);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x, b.position.y});
    t.setPosition(pos);
    rw.draw(t);
}

static void drawPanel(sf::RenderWindow& rw, sf::FloatRect rect, sf::Color fill) {
    sf::RectangleShape s({rect.size.x, rect.size.y});
    s.setPosition({rect.position.x, rect.position.y});
    s.setFillColor(fill);
    s.setOutlineThickness(1.f);
    s.setOutlineColor(sf::Color(255, 255, 255, 18));
    rw.draw(s);
}
#endif

void GUIView::drawLeftPanel(sf::RenderWindow& rw, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float lpW  = W * LP_W_FRAC;
    const float botH = H * BOT_H_FRAC;
    const float panH = H - botH;
    const float pad  = lpW * 0.07f;

    unsigned bodySz = static_cast<unsigned>(lpW * 0.10f);
    unsigned smSz   = static_cast<unsigned>(lpW * 0.085f);

    const float sumCardH = panH * 0.38f;
    const float plrCardH = panH - sumCardH;

    const sf::Texture* sumTex = am.texture("assets/components/card/GameSummaryCard.png");
    const sf::Texture* plrTex = am.texture("assets/components/card/PlayerCard.png");

    if (sumTex) {
        drawSprite(rw, sumTex, {{0.f, 0.f}, {lpW, sumCardH}});
    } else {
        drawPanel(rw, {{0.f, 0.f}, {lpW, sumCardH}}, sf::Color(0x17, 0x28, 0x48, 200));
        unsigned hSz2 = static_cast<unsigned>(lpW * 0.13f);
        drawLabel(rw, am, "bold", "GAME SUMMARY", hSz2,
                  sf::Color(160, 200, 240), {pad, pad * 0.6f});
    }

    if (plrTex) {
        drawSprite(rw, plrTex, {{0.f, sumCardH}, {lpW, plrCardH}});
    } else {
        drawPanel(rw, {{0.f, sumCardH}, {lpW, plrCardH}}, sf::Color(0x10, 0x1c, 0x38, 230));
        unsigned hSz2 = static_cast<unsigned>(lpW * 0.13f);
        drawLabel(rw, am, "bold", "PLAYERS", hSz2,
                  sf::Color(160, 200, 240), {pad, sumCardH + pad * 0.6f});
    }

    // Game Summary content — below header strip (~17% of sumCardH)
    {
        float y = sumCardH * 0.17f;

        std::string roundStr = "Round  " + std::to_string(state.currentTurn)
                             + " / " + std::to_string(state.maxTurn);
        drawLabel(rw, am, "regular", roundStr, bodySz,
                  sf::Color(55, 75, 110), {pad, y});
        y += bodySz * 1.8f;

        const sf::Color tcols[] = {
            sf::Color(0x00,0xc8,0xff), sf::Color(0xff,0x2d,0x8a),
            sf::Color(0xff,0xf2,0x00), sf::Color(0x00,0xff,0xb0)
        };
        float tx   = pad;
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

    // Players content — below header strip (~17% of plrCardH), starting at sumCardH
    {
        float contentY = sumCardH + plrCardH * 0.17f;
        float y = contentY;

        const sf::Color tcols[] = {
            sf::Color(0x00,0xc8,0xff), sf::Color(0xff,0x2d,0x8a),
            sf::Color(0xff,0xf2,0x00), sf::Color(0x00,0xff,0xb0)
        };
        float rowH = (panH - contentY - pad) / std::max(1, static_cast<int>(state.players.size()));
        rowH = std::min(rowH, panH * 0.18f);

        for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
            const auto& pv = state.players[static_cast<size_t>(i)];
            bool active   = (pv.username == state.currentPlayerName);
            bool bankrupt = (pv.status == PlayerStatus::BANKRUPT);

            if (active) {
                sf::RectangleShape hl({lpW - pad * 0.5f, rowH - pad * 0.2f});
                hl.setPosition({pad * 0.25f, y});
                hl.setFillColor(sf::Color(tcols[i % 4].r, tcols[i % 4].g, tcols[i % 4].b, 40));
                hl.setOutlineThickness(1.f);
                hl.setOutlineColor(sf::Color(tcols[i % 4].r, tcols[i % 4].g, tcols[i % 4].b, 100));
                rw.draw(hl);
            }

            float dotR = rowH * 0.22f;
            sf::CircleShape dot(dotR);
            dot.setFillColor(bankrupt ? sf::Color(160, 160, 170) : tcols[i % 4]);
            dot.setOrigin({dotR, dotR});
            dot.setPosition({pad + dotR, y + rowH * 0.3f});
            rw.draw(dot);

            sf::Color nameCol = bankrupt ? sf::Color(140, 145, 155)
                              : active   ? sf::Color(20, 35, 60)
                                         : sf::Color(50, 70, 100);
            drawLabel(rw, am, "bold", pv.username, bodySz, nameCol,
                      {pad + dotR * 2.8f, y + rowH * 0.08f});

            std::string moneyStr = "M" + std::to_string(pv.money.getAmount());
            drawLabel(rw, am, "regular", moneyStr, smSz,
                      bankrupt ? sf::Color(140, 145, 155) : sf::Color(20, 110, 70),
                      {pad + dotR * 2.8f, y + rowH * 0.52f});

            std::string propStr = std::to_string(pv.propertyCount) + " prop";
            drawLabel(rw, am, "regular", propStr, smSz,
                      sf::Color(90, 110, 145),
                      {pad + dotR * 2.8f + lpW * 0.28f, y + rowH * 0.52f});

            if (bankrupt) {
                drawLabel(rw, am, "bold", "BANKRUPT", smSz,
                          sf::Color(180, 50, 50), {pad, y + rowH * 0.75f});
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
    const float pad = rpW * 0.06f;

    drawPanel(rw, {{rpX, 0.f}, {rpW, H}}, sf::Color(0xdc, 0xe3, 0xeb, 200));

    unsigned hSz  = static_cast<unsigned>(rpW * 0.10f);
    unsigned smSz = static_cast<unsigned>(rpW * 0.072f);

    float y = pad;

    // Dice
    {
        drawLabel(rw, am, "bold", "DICE", hSz,
                  sf::Color(40, 60, 90), {rpX + pad, y});
        y += hSz * 1.5f;

        float diceH = H * 0.075f;
        float diceW = (rpW - pad * 2.8f) * 0.45f;
        sf::Color diceFill = diceRolled_ ? sf::Color(0xb0, 0xc4, 0xd8)
                                         : sf::Color(0xd0, 0xda, 0xe8, 200);
        for (int d = 0; d < 2; ++d) {
            float dx = rpX + pad + d * (diceW + pad * 0.8f);
            sf::RectangleShape box({diceW, diceH});
            box.setPosition({dx, y});
            box.setFillColor(diceFill);
            box.setOutlineThickness(1.f);
            box.setOutlineColor(sf::Color(100, 130, 170, 150));
            rw.draw(box);
            int val = diceRolled_ ? (d == 0 ? lastD1_ : lastD2_) : 0;
            std::string dStr = diceRolled_ ? std::to_string(val) : "-";
            sf::Text dt(am.font("bold"), dStr, static_cast<unsigned>(diceH * 0.6f));
            dt.setFillColor(sf::Color(30, 50, 80));
            auto b = dt.getLocalBounds();
            dt.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
            dt.setPosition({dx + diceW * 0.5f, y + diceH * 0.5f});
            rw.draw(dt);
        }
        y += diceH + pad * 1.2f;
    }

    // Game Log with GameLogCard.png
    {
        float logH = H * 0.36f;
        const sf::Texture* logCardTex = am.texture("assets/components/card/GameLogCard.png");
        if (logCardTex) {
            drawSprite(rw, logCardTex, {{rpX + pad * 0.5f, y}, {rpW - pad, logH}});
        } else {
            drawPanel(rw, {{rpX + pad * 0.5f, y}, {rpW - pad, logH}},
                      sf::Color(0x0d, 0x1a, 0x30, 230));
            drawLabel(rw, am, "bold", "GAME LOG", hSz,
                      sf::Color(160, 200, 240), {rpX + pad, y + pad * 0.5f});
        }

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
            drawLabel(rw, am, "regular", line, smSz, lc,
                      {rpX + pad, logContentY + (li - start) * lineH});
        }
        if (log_.empty()) {
            drawLabel(rw, am, "regular", "No events yet", smSz,
                      sf::Color(120, 150, 190),
                      {rpX + pad, logContentY});
        }
        y += logH + pad;
    }

    // Action buttons with EmptyRectBtn.png
    {
        drawLabel(rw, am, "bold", "ACTIONS", hSz,
                  sf::Color(40, 60, 90), {rpX + pad, y});
        y += hSz * 1.4f;

        const char* actions[] = {"Buy", "Build", "Mortgage", "Card"};
        float btnW = (rpW - pad * 2.6f) * 0.48f;
        float btnH = H * 0.062f;
        float gapX = pad * 0.6f;
        float gapY = pad * 0.5f;
        const sf::Texture* rectBtnTex = am.texture("assets/components/btn/EmptyRectBtn.png");
        for (int i = 0; i < 4; ++i) {
            int   col = i % 2, row = i / 2;
            float bx  = rpX + pad + col * (btnW + gapX);
            float by  = y + row * (btnH + gapY);
            if (rectBtnTex) {
                drawSprite(rw, rectBtnTex, {{bx, by}, {btnW, btnH}});
                unsigned csz = static_cast<unsigned>(btnH * 0.40f);
                sf::Text t(am.font("bold"), actions[i], csz);
                t.setFillColor(sf::Color(45, 65, 100));
                auto b = t.getLocalBounds();
                t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
                t.setPosition({bx + btnW * 0.5f, by + btnH * 0.5f});
                rw.draw(t);
            } else {
                drawMenuButton(rw, am, actions[i],
                               {bx + btnW * 0.5f, by + btnH * 0.5f}, {btnW, btnH}, false);
            }
        }
        y += 2 * (H * 0.062f + pad * 0.5f) + pad;
    }

    // Lempar Dadu button
    {
        float btnW = rpW - pad * 2.f;
        float btnH = H * 0.082f;
        float bx   = rpX + rpW * 0.5f;
        float by   = H - pad - btnH * 0.5f;
        bool disabled = diceRolled_;

        const sf::Texture* rectBtnTex = am.texture("assets/components/btn/EmptyRectBtn.png");
        if (rectBtnTex && !disabled) {
            drawSprite(rw, rectBtnTex,
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

        unsigned tsz = static_cast<unsigned>(btnH * 0.40f);
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
    const float stripW = W - rpW;
    const float stripY = H - botH;
    const float pad    = botH * 0.10f;

    const sf::Texture* propCardTex = am.texture("assets/components/card/YourPropertyCard.png");
    if (propCardTex) {
        drawSprite(rw, propCardTex, {{0.f, stripY}, {stripW, botH}});
    } else {
        drawPanel(rw, {{0.f, stripY}, {stripW, botH}}, sf::Color(0x0f, 0x1a, 0x32, 220));
        unsigned hSz2 = static_cast<unsigned>(botH * 0.18f);
        drawLabel(rw, am, "bold", "YOUR PROPERTY", hSz2,
                  sf::Color(160, 200, 240), {pad, stripY + pad * 0.8f});
    }

    // Content starts below header strip (~28% from top of YourPropertyCard)
    float cardY = stripY + botH * 0.28f;
    const float cardH   = botH - (botH * 0.28f) - pad;
    const float cardW   = cardH * 0.72f;
    const float cardGap = cardW * 0.18f;
    float cx   = pad;
    float maxX = stripW - pad;

    std::vector<const PropertyView*> myProps;
    for (const auto& pv : state.properties) {
        if (pv.ownerName == state.currentPlayerName)
            myProps.push_back(&pv);
    }

    if (myProps.empty() && !propCardTex) {
        drawLabel(rw, am, "regular", "No properties owned",
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
        sf::Color sc = tileColor(col);
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

void GUIView::showDiceResult(int d1, int d2, const string& playerName) {
    lastD1_ = d1;
    lastD2_ = d2;
    diceRolled_ = true;
    (void)playerName;
}

void GUIView::showPlayerLanding(const string& playerName, const string& tileName) {
    (void)playerName;
    (void)tileName;
}

void GUIView::showPropertyCard(const PropertyInfo& propertyInfo) {
    (void)propertyInfo;
}

void GUIView::showPlayerProperties(const vector<PropertyInfo>& list) {
    (void)list;
}

void GUIView::showBuyPrompt(const PropertyInfo& propertyInfo, Money playerMoney) {
    (void)propertyInfo;
    (void)playerMoney;
}

void GUIView::showRentPayment(const RentInfo& rentInfo) {
    (void)rentInfo;
}

void GUIView::showTaxPrompt(const TaxInfo& taxInfo) {
    (void)taxInfo;
}

void GUIView::showAuctionState(const AuctionState& auctionState) {
    (void)auctionState;
}

void GUIView::showFestivalPrompt(const vector<PropertyInfo>& ownedProperties) {
    (void)ownedProperties;
}

void GUIView::showBankruptcy(const BankruptcyInfo& bankruptcyInfo) {
    (void)bankruptcyInfo;
}

void GUIView::showLiquidationPanel(const LiquidationState& liquidationState) {
    (void)liquidationState;
}

void GUIView::showCardDrawn(const CardInfo& cardInfo) {
    (void)cardInfo;
}

void GUIView::showSkillCardHand(const vector<CardInfo>& cards) {
    (void)cards;
}

void GUIView::showTransactionLog(const vector<LogEntry>& entries) {
    log_ = entries;
}

void GUIView::showWinner(const WinnerInfo& winInfo) {
    (void)winInfo;
}

void GUIView::showJailStatus(const JailInfo& jailInfo) {
    (void)jailInfo;
}

void GUIView::showMessage(const string& message) {
    (void)message;
}

void GUIView::showBuildMenu(const BuildMenuState& buildMenuState) {
    (void)buildMenuState;
}

void GUIView::showMortgageMenu(const MortgageMenuState& mortgageMenuState) {
    (void)mortgageMenuState;
}

void GUIView::showRedeemMenu(const RedeemMenuState& redeemMenuState) {
    (void)redeemMenuState;
}

void GUIView::showDropCardPrompt(const vector<CardInfo>& cards) {
    (void)cards;
}

void GUIView::showSaveLoadStatus(const string& message) {
    (void)message;
}

void GUIView::showTurnInfo(const string& playerName, int turnNum, int maxTurn) {
    diceRolled_ = false;
    (void)playerName; (void)turnNum; (void)maxTurn;
}

void GUIView::showMainMenu() {}

void GUIView::showPlayerOrder(const vector<string>& orderedNames) {
    (void)orderedNames;
}

void GUIView::showDoubleBonusTurn(const string& playerName, int doubleCount) {
    (void)playerName;
    (void)doubleCount;
}

void GUIView::showAuctionWinner(const AuctionSummary& summary) {
    (void)summary;
}

void GUIView::showFestivalReinforced(const FestivalEffectInfo& info) {
    (void)info;
}

void GUIView::showFestivalAtMax(const FestivalEffectInfo& info) {
    (void)info;
}

void GUIView::showJailEntry(const JailEntryInfo& info) {
    (void)info;
}

void GUIView::showCardEffect(const CardEffectInfo& info) {
    (void)info;
}

void GUIView::showLiquidationResult(bool canCover, Money finalBalance) {
    (void)canCover;
    (void)finalBalance;
}
