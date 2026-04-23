#include "ui/GUIView.hpp"

#include "ui/AssetManager.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>

#include <string>

#include "gui/components/GUIViewDraw.hpp"
#endif

GUIView::GUIView(sf::RenderWindow& window) : window(&window) {}

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
    if (globeTex) {
        gui::draw::drawSpriteCover(rw, globeTex);
    } else {
        gui::draw::drawGlobeBackground(rw);
    }

    const sf::Texture* titleTex = am.texture("assets/bg/Title.png");
    if (titleTex) {
        auto tsz = titleTex->getSize();
        float base = (W < H) ? W : H;
        float titleW = base * 0.55f;
        float titleH = titleW * static_cast<float>(tsz.y) / static_cast<float>(tsz.x);
        sf::RenderStates addBlend;
        addBlend.blendMode = sf::BlendAdd;
        gui::draw::drawSprite(rw, titleTex,
                      {{cx - titleW * 0.5f, cy - H * 0.04f - titleH * 0.5f},
                               {titleW, titleH}},
                              addBlend);
    } else {
        unsigned titleSz = static_cast<unsigned>(H * 0.12f);
        gui::draw::drawCenteredText(rw, am, "title", "NIMONS", titleSz,
                                    sf::Color(255, 255, 255), cy - H * 0.10f);
        gui::draw::drawCenteredText(rw, am, "title", "POLY",   titleSz,
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

    unsigned subSz = static_cast<unsigned>(H * 0.022f);
    gui::draw::drawCenteredText(rw, am, "title", "presented by BurntCheesecake", subSz,
                                sf::Color(210, 220, 235), cy + H * 0.12f);

    const char* items[] = {"New Game", "Load Game", "Credits", "Exit"};
    const float btnH = H * 0.058f;
    const float startY = cy + H * 0.22f;
    const float gap    = H * 0.072f;
    for (int i = 0; i < 4; ++i) {
        bool  hov = hoveredItem_ == i;
        sf::Color tc = hov ? sf::Color(255, 255, 255) : sf::Color(210, 220, 235);
        unsigned csz = static_cast<unsigned>(btnH * 0.92f);
        sf::Text t(am.font("title"), items[i], csz);
        t.setFillColor(tc);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({cx, startY + i * gap});
        rw.draw(t);
    }

    rw.display();
#endif
}
