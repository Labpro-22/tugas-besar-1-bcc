#pragma once

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>
#include <string>

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"

namespace gui::draw {
    sf::Color tileColor(Color c);
    sf::Color tokenColor(int playerIndex);

    void drawTileCard(sf::RenderWindow& rw,
                      sf::Vector2f pos, float tileSz,
                      const TileView& tv,
                      AssetManager& am);

    void drawGlobeBackground(sf::RenderWindow& rw);
    void drawGameBackground(sf::RenderWindow& rw);

    void drawCenteredText(sf::RenderWindow& rw, AssetManager& am,
                          const std::string& fontKey, const std::string& str,
                          unsigned charSz, sf::Color color, float y);

    void drawMenuButton(sf::RenderWindow& rw, AssetManager& am,
                        const std::string& label,
                        sf::Vector2f center, sf::Vector2f sz,
                        bool hovered, bool selected = false);

    void drawSprite(sf::RenderWindow& rw, const sf::Texture* tex,
                    sf::FloatRect dest,
                    sf::RenderStates rs = sf::RenderStates::Default);

    void drawSpriteCover(sf::RenderWindow& rw, const sf::Texture* tex);

    void drawLabel(sf::RenderWindow& rw, AssetManager& am,
                   const std::string& fontKey, const std::string& str,
                   unsigned sz, sf::Color col, sf::Vector2f pos);

    std::vector<std::string> wrapText(AssetManager& am,
                                      const std::string& fontKey,
                                      const std::string& text,
                                      unsigned charSize,
                                      float maxWidth);

    void drawPanel(sf::RenderWindow& rw, sf::FloatRect rect, sf::Color fill);
}
#endif
