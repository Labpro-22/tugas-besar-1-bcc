#include "ui/GUIView.hpp"

#include "core/state/header/GameStateView.hpp"
#include "ui/AssetManager.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <vector>

#include "components/GUIViewDraw.hpp"
#endif

[[maybe_unused]] static constexpr float LP_W_FRAC  = 0.16f;
[[maybe_unused]] static constexpr float RP_W_FRAC  = 0.16f;
[[maybe_unused]] static constexpr float BOT_H_FRAC = 0.20f;

#if NIMONSPOLY_ENABLE_SFML
// Dark premium palette
static const sf::Color BG_COLOR(0x06, 0x09, 0x12);
static const sf::Color PANEL_BG(0x08, 0x0c, 0x14);
static const sf::Color PANEL_BORDER(0x5c, 0xd6, 0xff, 40);
static const sf::Color ACCENT_CYAN(0x5c, 0xd6, 0xff);
static const sf::Color ACCENT_GOLD(0xff, 0xc1, 0x07);
static const sf::Color TEXT_PRIMARY(0xe8, 0xed, 0xf2);
static const sf::Color TEXT_MUTED(0x7a, 0x85, 0x98);
static const sf::Color TEXT_GREEN(0x00, 0xd9, 0x8e);
static const sf::Color BTN_BG(0x12, 0x1a, 0x2e);
static const sf::Color BTN_BORDER(0x2a, 0x3f, 0x5c);
// (separator borders removed — tiles sit flush)
#endif

void GUIView::showBoard(const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    sf::RenderWindow& rw = *window;
    AssetManager& am = AssetManager::get();

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);

    // Globe fills full height, square, centered horizontally.
    const float globeSize = H;
    const float globeX = (W - globeSize) * 0.5f;
    const float globeY = 0.f;

    // Side panels width adapts to leftover space.
    const float panelW = (W - globeSize) * 0.5f;

    // Board fills 100% of window height, tiles hugging the globe edge-to-edge.
    // Board fills full window, tiles use exact float size — no snapping, no gaps.
    const float boardSize = H;
    const sf::Vector2f boardOrigin{
        (W - boardSize) * 0.5f,
        0.f,
    };

    constexpr int EDGE = 10;
    constexpr int SIDE = EDGE + 1;
    constexpr int TOTAL = EDGE * 4;
    const float tileSz = boardSize / static_cast<float>(SIDE);

    auto tilePos = [&](int idx) -> sf::Vector2f {
        const float edge = tileSz * static_cast<float>(SIDE);
        const int   sd   = idx / EDGE;
        const int   off  = idx % EDGE;
        switch (sd) {
            case 0:  return {boardOrigin.x + edge - tileSz - off * tileSz, boardOrigin.y + edge - tileSz};
            case 1:  return {boardOrigin.x,                                 boardOrigin.y + edge - tileSz - off * tileSz};
            case 2:  return {boardOrigin.x + off * tileSz,                  boardOrigin.y};
            default: return {boardOrigin.x + edge - tileSz,                 boardOrigin.y + off * tileSz};
        }
    };

    rw.clear(BG_COLOR);

    // ---- Globe (behind everything) ----
    const sf::Texture* globeTex = am.texture("assets/bg/GlobeWShadow.png");
    if (globeTex) {
        gui::draw::drawSprite(rw, globeTex, {{globeX, globeY}, {globeSize, globeSize}});
    }

    // ---- Title (small, centered in the middle of the globe, raised 20px) ----
    const sf::Texture* titleTex = am.texture("assets/bg/Title.png");
    if (titleTex) {
        float titleW = W * 0.28f;
        float scale = titleW / static_cast<float>(titleTex->getSize().x);
        float titleH = static_cast<float>(titleTex->getSize().y) * scale;
        float titleX = (W - titleW) * 0.5f;
        float titleY = (H - titleH) * 0.5f - 20.f;
        gui::draw::drawSprite(rw, titleTex, {{titleX, titleY}, {titleW, titleH}});
    }

    // ---- Tiles (no borders/separators — tiles sit flush against each other) ----
    const int numTiles = std::min<int>(TOTAL, static_cast<int>(state.tiles.size()));
    for (int i = 0; i < numTiles; ++i)
        gui::draw::drawTileCard(rw, tilePos(i), tileSz, state.tiles[static_cast<size_t>(i)], am);

    // ---- Player tokens ----
    const float tokenR = tileSz * 0.16f;
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

    drawLeftPanel(rw, state, panelW, H);
    drawRightPanel(rw, state, panelW, H);

    // Save/load status toast
    if (saveLoadStatusFrames_ > 0) {
        --saveLoadStatusFrames_;
        const float toastW = W * 0.40f;
        const float toastH = static_cast<float>(rw.getSize().y) * 0.06f;
        const float tx = W * 0.5f;
        const float ty = static_cast<float>(rw.getSize().y) * 0.04f + toastH * 0.5f;
        sf::RectangleShape toast({toastW, toastH});
        toast.setOrigin({toastW * 0.5f, toastH * 0.5f});
        toast.setPosition({tx, ty});
        toast.setFillColor(sf::Color(30, 50, 80, 220));
        toast.setOutlineThickness(1.5f);
        toast.setOutlineColor(sf::Color(80, 130, 200));
        rw.draw(toast);

        unsigned tsz = static_cast<unsigned>(toastH * 0.40f);
        sf::Text t(am.font("regular"), saveLoadStatus_, tsz);
        t.setFillColor(sf::Color(230, 240, 255));
        auto tb = t.getLocalBounds();
        t.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        t.setPosition({tx, ty});
        rw.draw(t);
    }

    if (currentPrompt_ && currentPrompt_->type != GUIPromptType::NONE && !currentPrompt_->resolved) {
        renderPromptOverlay(*currentPrompt_);
    }

    rw.display();
#else
    (void)state;
#endif
}

void GUIView::drawLeftPanel(sf::RenderWindow& rw, const GameStateView& state,
                            float panelW, float H) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    const float pad = 14.f;

    // Panel background
    sf::RectangleShape panel({panelW, H});
    panel.setPosition({0.f, 0.f});
    panel.setFillColor(PANEL_BG);
    rw.draw(panel);

    // Right edge subtle glow line
    sf::RectangleShape edgeLine({2.f, H});
    edgeLine.setPosition({panelW - 1.f, 0.f});
    edgeLine.setFillColor(PANEL_BORDER);
    rw.draw(edgeLine);

    float y = pad;

    // ---- Title ----
    {
        sf::Text t(am.font("extrabold"), "NIMONSPOLY", static_cast<unsigned>(panelW * 0.13f));
        t.setFillColor(ACCENT_CYAN);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
        t.setPosition({panelW * 0.5f, y});
        rw.draw(t);
        y += panelW * 0.13f + 6.f;
    }

    // ---- Round info ----
    {
        std::string roundStr = "Round  " + std::to_string(state.currentTurn)
                             + " / " + std::to_string(state.maxTurn);
        sf::Text t(am.font("regular"), roundStr, static_cast<unsigned>(panelW * 0.07f));
        t.setFillColor(TEXT_MUTED);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
        t.setPosition({panelW * 0.5f, y});
        rw.draw(t);
        y += panelW * 0.07f + 18.f;
    }

    // ---- Separator ----
    {
        sf::RectangleShape sep({panelW - pad * 2.f, 1.f});
        sep.setPosition({pad, y});
        sep.setFillColor(sf::Color(255, 255, 255, 25));
        rw.draw(sep);
        y += 16.f;
    }

    // ---- Player List ----
    const sf::Color tcols[] = {
        sf::Color(0x00,0xc8,0xff), sf::Color(0xff,0x2d,0x8a),
        sf::Color(0xff,0xf2,0x00), sf::Color(0x00,0xff,0xb0)
    };

    float rowH = 42.f;
    for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
        const auto& pv = state.players[static_cast<size_t>(i)];
        bool active   = (pv.username == state.currentPlayerName);
        bool bankrupt = (pv.status == PlayerStatus::BANKRUPT);

        // Active highlight bar
        if (active) {
            sf::RectangleShape hl({panelW - pad * 0.5f, rowH});
            hl.setPosition({pad * 0.25f, y});
            hl.setFillColor(sf::Color(tcols[i % 4].r, tcols[i % 4].g, tcols[i % 4].b, 35));
            hl.setOutlineThickness(1.f);
            hl.setOutlineColor(sf::Color(tcols[i % 4].r, tcols[i % 4].g, tcols[i % 4].b, 90));
            rw.draw(hl);
        }

        // Color dot
        float dotR = 6.f;
        sf::CircleShape dot(dotR);
        dot.setFillColor(bankrupt ? sf::Color(100, 100, 110) : tcols[i % 4]);
        dot.setOrigin({dotR, dotR});
        dot.setPosition({pad + dotR + 4.f, y + rowH * 0.5f});
        rw.draw(dot);

        // Name
        std::string displayName = pv.username;
        if (displayName.size() > 12) displayName = displayName.substr(0, 10) + "..";
        sf::Text nameTxt(am.font("bold"), displayName, static_cast<unsigned>(panelW * 0.065f));
        nameTxt.setFillColor(bankrupt ? sf::Color(100, 105, 115) : (active ? TEXT_PRIMARY : TEXT_MUTED));
        nameTxt.setPosition({pad + dotR * 2.f + 12.f, y + 4.f});
        rw.draw(nameTxt);

        // Money
        std::string moneyStr = "M" + std::to_string(pv.money.getAmount());
        sf::Text moneyTxt(am.font("regular"), moneyStr, static_cast<unsigned>(panelW * 0.055f));
        moneyTxt.setFillColor(bankrupt ? sf::Color(100, 105, 115) : TEXT_GREEN);
        moneyTxt.setPosition({pad + dotR * 2.f + 12.f, y + rowH * 0.52f});
        rw.draw(moneyTxt);

        // Bankrupt tag
        if (bankrupt) {
            sf::Text tag(am.font("bold"), "BANKRUPT", static_cast<unsigned>(panelW * 0.05f));
            tag.setFillColor(sf::Color(0xff, 0x4d, 0x4d));
            tag.setPosition({panelW - pad - 60.f, y + rowH * 0.3f});
            rw.draw(tag);
        }

        y += rowH + 4.f;
    }

    y += 10.f;

    // ---- Separator ----
    {
        sf::RectangleShape sep({panelW - pad * 2.f, 1.f});
        sep.setPosition({pad, y});
        sep.setFillColor(sf::Color(255, 255, 255, 25));
        rw.draw(sep);
        y += 16.f;
    }

    // ---- Game Log ----
    {
        sf::Text hdr(am.font("bold"), "GAME LOG", static_cast<unsigned>(panelW * 0.07f));
        hdr.setFillColor(TEXT_PRIMARY);
        auto hb = hdr.getLocalBounds();
        hdr.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
        hdr.setPosition({panelW * 0.5f, y});
        rw.draw(hdr);
        y += panelW * 0.07f + 10.f;

        int total = static_cast<int>(log_.size());
        float lineH = 18.f;
        float maxLogH = H - y - pad;
        float usedH = 0;
        struct Entry { int idx; std::vector<std::string> lines; };
        std::vector<Entry> visible;

        for (int li = total - 1; li >= 0; --li) {
            const auto& e = log_[static_cast<size_t>(li)];
            std::string full = "[" + e.username + "] " + e.detail;
            auto wrapped = gui::draw::wrapText(am, "regular", full,
                                               static_cast<unsigned>(panelW * 0.055f),
                                               panelW - pad * 2.f);
            float h = wrapped.size() * lineH;
            if (usedH + h > maxLogH) break;
            visible.push_back({li, wrapped});
            usedH += h;
        }

        float drawY = y;
        for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
            sf::Color lc = (it->idx == total - 1) ? TEXT_PRIMARY : TEXT_MUTED;
            for (const auto& wl : it->lines) {
                sf::Text lt(am.font("regular"), wl, static_cast<unsigned>(panelW * 0.055f));
                lt.setFillColor(lc);
                lt.setPosition({pad, drawY});
                rw.draw(lt);
                drawY += lineH;
            }
        }
        if (log_.empty()) {
            sf::Text lt(am.font("regular"), "No events yet", static_cast<unsigned>(panelW * 0.055f));
            lt.setFillColor(TEXT_MUTED);
            lt.setPosition({pad, drawY});
            rw.draw(lt);
        }
    }
#else
    (void)rw; (void)state; (void)panelW; (void)H;
#endif
}

void GUIView::drawRightPanel(sf::RenderWindow& rw, const GameStateView& state,
                             float panelW, float H) {
#if NIMONSPOLY_ENABLE_SFML
    AssetManager& am = AssetManager::get();
    const float W = static_cast<float>(rw.getSize().x);
    const float px = W - panelW;
    const float pad = 14.f;

    // Panel background
    sf::RectangleShape panel({panelW, H});
    panel.setPosition({px, 0.f});
    panel.setFillColor(PANEL_BG);
    rw.draw(panel);

    // Left edge subtle glow line
    sf::RectangleShape edgeLine({2.f, H});
    edgeLine.setPosition({px, 0.f});
    edgeLine.setFillColor(PANEL_BORDER);
    rw.draw(edgeLine);

    float y = pad;

    // ---- Action Buttons Grid 2x3 ----
    const char* actionLabels[] = {"TEBUS", "BANGUN", "GADAI", "KARTU", "SIMPAN", "SELESAI"};
    float btnW = (panelW - pad * 2.f - 8.f) * 0.5f;
    float btnH = 40.f;
    float gapX = 8.f;
    float gapY = 8.f;

    for (int i = 0; i < 6; ++i) {
        int col = i % 2;
        int row = i / 2;
        float bx = px + pad + col * (btnW + gapX);
        float by = y + row * (btnH + gapY);

        sf::RectangleShape btn({btnW, btnH});
        btn.setPosition({bx, by});
        btn.setFillColor(BTN_BG);
        btn.setOutlineThickness(1.f);
        btn.setOutlineColor(BTN_BORDER);
        rw.draw(btn);

        sf::Text t(am.font("bold"), actionLabels[i], static_cast<unsigned>(btnH * 0.38f));
        t.setFillColor(TEXT_PRIMARY);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({bx + btnW * 0.5f, by + btnH * 0.5f});
        rw.draw(t);
    }
    y += 3.f * (btnH + gapY) + 18.f;

    // ---- Separator ----
    {
        sf::RectangleShape sep({panelW - pad * 2.f, 1.f});
        sep.setPosition({px + pad, y});
        sep.setFillColor(sf::Color(255, 255, 255, 25));
        rw.draw(sep);
        y += 18.f;
    }

    // ---- Dice Button ----
    {
        bool disabled = state.hasRolledDice;
        float dbW = panelW - pad * 2.f;
        float dbH = 56.f;
        float dbX = px + pad;
        float dbY = y;

        sf::RectangleShape db({dbW, dbH});
        db.setPosition({dbX, dbY});
        db.setFillColor(disabled ? sf::Color(0x12, 0x1a, 0x2e) : sf::Color(0x1a, 0x28, 0x45));
        db.setOutlineThickness(disabled ? 1.f : 2.f);
        db.setOutlineColor(disabled ? BTN_BORDER : ACCENT_GOLD);
        rw.draw(db);

        sf::Text t(am.font("title"), "LEMPAR DADU", static_cast<unsigned>(dbH * 0.42f));
        t.setFillColor(disabled ? TEXT_MUTED : TEXT_PRIMARY);
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
        t.setPosition({dbX + dbW * 0.5f, dbY + dbH * 0.5f});
        rw.draw(t);
        y += dbH + 18.f;
    }

    // ---- Separator ----
    {
        sf::RectangleShape sep({panelW - pad * 2.f, 1.f});
        sep.setPosition({px + pad, y});
        sep.setFillColor(sf::Color(255, 255, 255, 25));
        rw.draw(sep);
        y += 18.f;
    }

    // ---- Property Mini-Cards ----
    {
        sf::Text hdr(am.font("bold"), "YOUR PROPERTIES", static_cast<unsigned>(panelW * 0.07f));
        hdr.setFillColor(TEXT_PRIMARY);
        auto hb = hdr.getLocalBounds();
        hdr.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
        hdr.setPosition({px + panelW * 0.5f, y});
        rw.draw(hdr);
        y += panelW * 0.07f + 12.f;

        std::vector<const PropertyView*> myProps;
        for (const auto& pv : state.properties) {
            if (pv.ownerName == state.currentPlayerName)
                myProps.push_back(&pv);
        }

        if (myProps.empty()) {
            sf::Text empty(am.font("regular"), "No properties owned",
                           static_cast<unsigned>(panelW * 0.06f));
            empty.setFillColor(TEXT_MUTED);
            empty.setPosition({px + pad, y});
            rw.draw(empty);
        } else {
            float cardW = (panelW - pad * 2.f - 8.f) / 3.f;
            float cardH = cardW * 1.35f;
            float cardGap = 4.f;

            for (size_t idx = 0; idx < myProps.size() && idx < 6; ++idx) {
                const auto* pv = myProps[idx];
                int col = static_cast<int>(idx % 3);
                int row = static_cast<int>(idx / 3);
                float cx = px + pad + col * (cardW + cardGap);
                float cy = y + row * (cardH + cardGap);

                sf::RectangleShape card({cardW, cardH});
                card.setPosition({cx, cy});
                card.setFillColor(sf::Color(0x0d, 0x14, 0x24));
                card.setOutlineThickness(1.f);
                card.setOutlineColor(BTN_BORDER);
                rw.draw(card);

                Color colGrp = Color::DEFAULT;
                for (const auto& tv : state.tiles) {
                    if (tv.code == pv->code) { colGrp = tv.color; break; }
                }
                sf::Color sc = gui::draw::tileColor(colGrp);
                if (colGrp != Color::DEFAULT) {
                    sf::RectangleShape strip({cardW, cardH * 0.22f});
                    strip.setPosition({cx, cy});
                    strip.setFillColor(sc);
                    rw.draw(strip);
                }

                sf::Text ct(am.font("bold"), pv->code, static_cast<unsigned>(cardH * 0.22f));
                ct.setFillColor(colGrp != Color::DEFAULT ? sc : TEXT_MUTED);
                if (colGrp == Color::YELLOW) ct.setFillColor(sf::Color(0x90, 0x87, 0x00));
                auto b = ct.getLocalBounds();
                ct.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
                ct.setPosition({cx + cardW * 0.5f, cy + cardH * 0.58f});
                rw.draw(ct);

                if (pv->buildingLevel > 0) {
                    float dotR = 3.5f;
                    float dotY = cy + cardH * 0.82f;
                    int lvl = std::min(pv->buildingLevel, 5);
                    float startDX = cx + cardW * 0.5f - (lvl - 1) * dotR * 1.6f;
                    for (int d = 0; d < lvl; ++d) {
                        sf::CircleShape dot(dotR);
                        dot.setFillColor(pv->status == PropertyStatus::MORTGAGED
                                         ? sf::Color(150, 150, 150) : sf::Color(0x00, 0xd9, 0x8e));
                        dot.setOrigin({dotR, dotR});
                        dot.setPosition({startDX + d * dotR * 3.2f, dotY});
                        rw.draw(dot);
                    }
                }

                if (pv->status == PropertyStatus::MORTGAGED) {
                    sf::RectangleShape ov({cardW, cardH});
                    ov.setPosition({cx, cy});
                    ov.setFillColor(sf::Color(0, 0, 0, 120));
                    rw.draw(ov);
                    sf::Text mt(am.font("bold"), "MRTG", static_cast<unsigned>(cardH * 0.16f));
                    mt.setFillColor(sf::Color(0xff, 0x4d, 0x4d));
                    auto mb = mt.getLocalBounds();
                    mt.setOrigin({mb.position.x + mb.size.x * 0.5f, mb.position.y + mb.size.y * 0.5f});
                    mt.setPosition({cx + cardW * 0.5f, cy + cardH * 0.5f});
                    rw.draw(mt);
                }
            }
        }
    }
#else
    (void)rw; (void)state; (void)panelW; (void)H;
#endif
}

void GUIView::drawDieFace(sf::RenderWindow& rw, float cx, float cy, float size, int face,
                          unsigned fillRGB, unsigned dotRGB) {
#if NIMONSPOLY_ENABLE_SFML
    float half = size * 0.5f;
    float radius = size * 0.08f;
    float offset = size * 0.28f;

    auto toColor = [](unsigned rgb) -> sf::Color {
        return sf::Color(static_cast<uint8_t>((rgb >> 16) & 0xFF),
                         static_cast<uint8_t>((rgb >> 8) & 0xFF),
                         static_cast<uint8_t>(rgb & 0xFF));
    };
    sf::Color fill = toColor(fillRGB);
    sf::Color dot = toColor(dotRGB);

    sf::RectangleShape bg({size, size});
    bg.setOrigin({half, half});
    bg.setPosition({cx, cy});
    bg.setFillColor(fill);
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(80, 90, 110));
    rw.draw(bg);

    auto drawDot = [&](float dx, float dy) {
        sf::CircleShape d(radius);
        d.setFillColor(dot);
        d.setOrigin({radius, radius});
        d.setPosition({cx + dx, cy + dy});
        rw.draw(d);
    };

    if (face == 1) {
        drawDot(0, 0);
    } else if (face == 2) {
        drawDot(-offset, -offset);
        drawDot(offset, offset);
    } else if (face == 3) {
        drawDot(-offset, -offset);
        drawDot(0, 0);
        drawDot(offset, offset);
    } else if (face == 4) {
        drawDot(-offset, -offset);
        drawDot(offset, -offset);
        drawDot(-offset, offset);
        drawDot(offset, offset);
    } else if (face == 5) {
        drawDot(-offset, -offset);
        drawDot(offset, -offset);
        drawDot(0, 0);
        drawDot(-offset, offset);
        drawDot(offset, offset);
    } else if (face == 6) {
        drawDot(-offset, -offset);
        drawDot(offset, -offset);
        drawDot(-offset, 0);
        drawDot(offset, 0);
        drawDot(-offset, offset);
        drawDot(offset, offset);
    }
#endif
}

void GUIView::drawDiceAnimation(sf::RenderWindow& rw, float dt) {
#if NIMONSPOLY_ENABLE_SFML
    diceAnimElapsed_ += dt;

    if (diceAnimElapsed_ >= DICE_ANIM_DURATION) {
        diceAnimating_ = false;
        diceAnimElapsed_ = 0.f;
        return;
    }

    const float W = static_cast<float>(rw.getSize().x);
    const float H = static_cast<float>(rw.getSize().y);
    const float dieSz = 90.f;
    const float gap = 30.f;
    const float totalW = dieSz * 2.f + gap;
    sf::Vector2f center{W * 0.5f, H * 0.5f};

    // Semi-transparent backdrop
    sf::RectangleShape dim({W, H});
    dim.setFillColor(sf::Color(0, 0, 0, 80));
    rw.draw(dim);

    // Cycle random faces during animation
    static std::mt19937 rng(static_cast<unsigned>(std::random_device{}()));
    std::uniform_int_distribution<int> dist(1, 6);
    if (static_cast<int>(diceAnimElapsed_ * 10) % 2 == 0) {
        diceAnimFace1_ = dist(rng);
        diceAnimFace2_ = dist(rng);
    }

    drawDieFace(rw, center.x - totalW * 0.5f + dieSz * 0.5f, center.y, dieSz, diceAnimFace1_,
                0xFAFAF8, 0x282832);
    drawDieFace(rw, center.x + totalW * 0.5f - dieSz * 0.5f, center.y, dieSz, diceAnimFace2_,
                0xFAFAF8, 0x282832);
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

    // Property card overlay (if buy prompt is active)
    float panelYOffset = 0.f;
    if (buyPromptActive_ && prompt.type == GUIPromptType::YES_NO) {
        const float cardW = W * 0.30f;
        const float cardH = H * 0.22f;
        const float cardY = cy - H * 0.22f;
        panelYOffset = cardH * 0.5f + 20.f;

        sf::RectangleShape card({cardW, cardH});
        card.setPosition({cx - cardW * 0.5f, cardY - cardH * 0.5f});
        card.setFillColor(sf::Color(250, 250, 248));
        card.setOutlineThickness(2.f);
        card.setOutlineColor(sf::Color(60, 90, 140));
        rw.draw(card);

        unsigned csz = static_cast<unsigned>(cardH * 0.13f);
        sf::Text code(am.font("bold"), buyPromptInfo_.code, csz);
        code.setFillColor(sf::Color(35, 55, 90));
        auto cb = code.getLocalBounds();
        code.setOrigin({cb.position.x + cb.size.x * 0.5f, cb.position.y});
        code.setPosition({cx, cardY - cardH * 0.32f});
        rw.draw(code);

        unsigned nsz = static_cast<unsigned>(cardH * 0.10f);
        sf::Text name(am.font("regular"), buyPromptInfo_.name, nsz);
        name.setFillColor(sf::Color(80, 100, 130));
        auto nb = name.getLocalBounds();
        name.setOrigin({nb.position.x + nb.size.x * 0.5f, nb.position.y});
        name.setPosition({cx, cardY - cardH * 0.12f});
        rw.draw(name);

        std::string priceStr = "Harga: " + buyPromptInfo_.purchasePrice.toString();
        sf::Text price(am.font("regular"), priceStr, nsz);
        price.setFillColor(sf::Color(20, 110, 70));
        auto pb = price.getLocalBounds();
        price.setOrigin({pb.position.x + pb.size.x * 0.5f, pb.position.y});
        price.setPosition({cx, cardY + cardH * 0.08f});
        rw.draw(price);

        std::string moneyStr = "Uangmu: " + buyPromptMoney_.toString();
        sf::Text money(am.font("regular"), moneyStr, nsz);
        money.setFillColor(sf::Color(100, 100, 120));
        auto mb = money.getLocalBounds();
        money.setOrigin({mb.position.x + mb.size.x * 0.5f, mb.position.y});
        money.setPosition({cx, cardY + cardH * 0.24f});
        rw.draw(money);
    }

    // Panel
    const float panelW = W * 0.42f;
    const float panelH = H * 0.28f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({cx - panelW * 0.5f, cy - panelH * 0.5f + panelYOffset});
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
    title.setPosition({cx, cy - panelH * 0.30f + panelYOffset});
    rw.draw(title);

    // Type-specific content
    float contentY = cy - panelH * 0.08f + panelYOffset;

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
    buffer.setPosition({cx, cy + panelH * 0.22f + panelYOffset});
    rw.draw(buffer);

    if (prompt.resolved) {
        buyPromptActive_ = false;
    }
#endif
}

void GUIView::handleInGameClick(float mx, float my, std::string& outCommand, const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;
    const float W = static_cast<float>(window->getSize().x);
    const float H = static_cast<float>(window->getSize().y);

    const float globeSize = H;
    const float panelW = (W - globeSize) * 0.5f;
    const float px = W - panelW;
    const float pad = 14.f;

    float y = pad;

    // Action Buttons Grid 2x3
    float btnW = (panelW - pad * 2.f - 8.f) * 0.5f;
    float btnH = 40.f;
    float gapX = 8.f;
    float gapY = 8.f;

    const char* actions[] = {"TEBUS", "BANGUN", "GADAI", "KARTU", "SIMPAN", "SELESAI"};
    for (int i = 0; i < 6; ++i) {
        int col = i % 2;
        int row = i / 2;
        float bx = px + pad + col * (btnW + gapX);
        float by = y + row * (btnH + gapY);
        if (mx >= bx && mx <= bx + btnW &&
            my >= by && my <= by + btnH) {
            outCommand = actions[i];
            return;
        }
    }
    y += 3.f * (btnH + gapY) + 18.f + 18.f; // after buttons + separator

    // Dice Button
    {
        float dbW = panelW - pad * 2.f;
        float dbH = 56.f;
        float dbX = px + pad;
        float dbY = y;
        if (mx >= dbX && mx <= dbX + dbW &&
            my >= dbY && my <= dbY + dbH) {
            if (!state.hasRolledDice && !diceAnimating_) {
                diceAnimating_ = true;
                diceAnimElapsed_ = 0.f;
                outCommand = "DADU";
            }
            return;
        }
    }
#endif
}
