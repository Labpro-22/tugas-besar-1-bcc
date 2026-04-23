#include "GUIViewDraw.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <algorithm>
#include <array>

namespace gui::draw {

sf::Color tileColor(Color c) {
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

sf::Color tokenColor(int playerIndex) {
    constexpr std::array<sf::Color, 4> colors{
        sf::Color(0x00, 0xc8, 0xff),
        sf::Color(0xff, 0x2d, 0x8a),
        sf::Color(0xff, 0xf2, 0x00),
        sf::Color(0x00, 0xff, 0xb0),
    };
    return colors[static_cast<size_t>(playerIndex) % 4];
}

void drawTileCard(sf::RenderWindow& rw,
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

    const sf::Texture* tex = am.tileTexture(tv.code);
    if (tex) {
        const float pad = tileSz * 0.06f;
        const float drawSz = tileSz - pad * 2.0f;
        auto texSz = tex->getSize();
        float scale = std::min(drawSz / static_cast<float>(texSz.x),
                               drawSz / static_cast<float>(texSz.y));
        sf::Sprite sprite(*tex);
        sprite.setScale({scale, scale});
        sprite.setPosition({center.x - texSz.x * scale * 0.5f,
                            center.y - texSz.y * scale * 0.5f});
        rw.draw(sprite, rs);
        return;
    }

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
    if (hasGroup) {
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

void drawGlobeBackground(sf::RenderWindow& rw) {
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

void drawGameBackground(sf::RenderWindow& rw) {
    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);

    sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
    quad[0].position = {0.f, 0.f};
    quad[1].position = {W, 0.f};
    quad[2].position = {0.f, H};
    quad[3].position = {W, H};
    quad[0].color = sf::Color(0xd8, 0xde, 0xe6);
    quad[1].color = sf::Color(0xd0, 0xd6, 0xe0);
    quad[2].color = sf::Color(0xc7, 0xcf, 0xda);
    quad[3].color = sf::Color(0xdf, 0xe4, 0xeb);
    rw.draw(quad);

    const float glowR = std::min(W, H) * 0.42f;
    sf::CircleShape glow(glowR);
    glow.setFillColor(sf::Color(255, 255, 255, 55));
    glow.setOrigin({glowR, glowR});
    glow.setPosition({W * 0.52f, H * 0.46f});
    rw.draw(glow);
}

void drawCenteredText(sf::RenderWindow& rw, AssetManager& am,
                      const std::string& fontKey, const std::string& str,
                      unsigned charSz, sf::Color color, float y) {
    sf::Text t(am.font(fontKey), str, charSz);
    t.setFillColor(color);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    t.setPosition({static_cast<float>(rw.getSize().x) * 0.5f, y});
    rw.draw(t);
}

void drawMenuButton(sf::RenderWindow& rw, AssetManager& am,
                    const std::string& label,
                    sf::Vector2f center, sf::Vector2f sz,
                    bool hovered, bool selected) {
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

void drawSprite(sf::RenderWindow& rw, const sf::Texture* tex,
                sf::FloatRect dest,
                sf::RenderStates rs) {
    if (!tex) return;
    auto tsz = tex->getSize();
    sf::Sprite sprite(*tex);
    sprite.setScale({dest.size.x / static_cast<float>(tsz.x),
                     dest.size.y / static_cast<float>(tsz.y)});
    sprite.setPosition({dest.position.x, dest.position.y});
    rw.draw(sprite, rs);
}

void drawSpriteCover(sf::RenderWindow& rw, const sf::Texture* tex) {
    if (!tex) return;
    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    auto tsz = tex->getSize();
    float scale = std::max(W / static_cast<float>(tsz.x),
                           H / static_cast<float>(tsz.y));
    float drawW = static_cast<float>(tsz.x) * scale;
    float drawH = static_cast<float>(tsz.y) * scale;
    sf::Sprite sprite(*tex);
    sprite.setScale({scale, scale});
    sprite.setPosition({(W - drawW) * 0.5f, (H - drawH) * 0.5f});
    rw.draw(sprite);
}

void drawLabel(sf::RenderWindow& rw, AssetManager& am,
               const std::string& fontKey, const std::string& str,
               unsigned sz, sf::Color col, sf::Vector2f pos) {
    sf::Text t(am.font(fontKey), str, sz);
    t.setFillColor(col);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x, b.position.y});
    t.setPosition(pos);
    rw.draw(t);
}

void drawPanel(sf::RenderWindow& rw, sf::FloatRect rect, sf::Color fill) {
    sf::RectangleShape s({rect.size.x, rect.size.y});
    s.setPosition({rect.position.x, rect.position.y});
    s.setFillColor(fill);
    s.setOutlineThickness(1.f);
    s.setOutlineColor(sf::Color(255, 255, 255, 18));
    rw.draw(s);
}

}  // namespace gui::draw
#endif
