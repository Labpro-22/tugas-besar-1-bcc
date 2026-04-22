#include "ui/GUIView.hpp"

#include "core/state/header/GameStateView.hpp"

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#endif

GUIView::GUIView(sf::RenderWindow& window) : window(&window) {}

#if NIMONSPOLY_ENABLE_SFML
static sf::Color tileColor(Color c) {
    switch (c) {
        case Color::BROWN:      return sf::Color(139, 90,  43);
        case Color::LIGHT_BLUE: return sf::Color( 90, 195, 245);
        case Color::PINK:       return sf::Color(220, 100, 180);
        case Color::ORANGE:     return sf::Color(230, 130,  40);
        case Color::RED:        return sf::Color(210,  50,  50);
        case Color::YELLOW:     return sf::Color(230, 210,  60);
        case Color::GREEN:      return sf::Color( 60, 170,  80);
        case Color::DARK_BLUE:  return sf::Color( 40,  80, 200);
        case Color::GRAY:       return sf::Color(160, 160, 160);
        default:                return sf::Color( 55,  60,  70);
    }
}

static sf::Color tokenColor(int playerIndex) {
    constexpr std::array<sf::Color, 4> colors{
        sf::Color(235,  97,  95),
        sf::Color(110, 200, 255),
        sf::Color(116, 214, 141),
        sf::Color(244, 196,  99),
    };
    return colors[static_cast<size_t>(playerIndex) % 4];
}
#endif

void GUIView::showBoard(const GameStateView& state) {
#if NIMONSPOLY_ENABLE_SFML
    if (!window) return;

    sf::RenderWindow& rw = *window;
    const sf::Vector2u winSize = rw.getSize();
    const float minDim   = std::min<float>(static_cast<float>(winSize.x),
                                            static_cast<float>(winSize.y));
    const float boardSz  = minDim * 0.86f;
    const sf::Vector2f origin{
        (static_cast<float>(winSize.x) - boardSz) * 0.5f,
        (static_cast<float>(winSize.y) - boardSz) * 0.5f,
    };

    constexpr int EDGE    = 10;
    constexpr int SIDE    = EDGE + 1;     // 11
    constexpr int TOTAL   = EDGE * 4;    // 40
    const float   tileSz  = boardSz / static_cast<float>(SIDE);
    const float   innerSz = boardSz - 2.0f * tileSz;

    auto tilePos = [&](int idx) -> sf::Vector2f {
        const float edge = tileSz * static_cast<float>(SIDE);
        const int   side = idx / EDGE;
        const int   off  = idx % EDGE;
        switch (side) {
            case 0: // bottom row right→left
                return {origin.x + edge - tileSz - off * tileSz, origin.y + edge - tileSz};
            case 1: // left col bottom→top
                return {origin.x, origin.y + edge - tileSz - off * tileSz};
            case 2: // top row left→right
                return {origin.x + off * tileSz, origin.y};
            default: // right col top→bottom
                return {origin.x + edge - tileSz, origin.y + off * tileSz};
        }
    };

    const float stripH = tileSz * 0.18f;

    rw.clear(sf::Color(20, 22, 28));

    sf::RectangleShape inner({innerSz, innerSz});
    inner.setPosition({origin.x + tileSz, origin.y + tileSz});
    inner.setFillColor(sf::Color(30, 35, 45));
    rw.draw(inner);

    const int numTiles = std::min<int>(TOTAL, static_cast<int>(state.tiles.size()));
    for (int i = 0; i < numTiles; ++i) {
        const sf::Vector2f pos = tilePos(i);
        const Color col = state.tiles[static_cast<size_t>(i)].color;

        // Tile background
        sf::RectangleShape bg({tileSz, tileSz});
        bg.setPosition(pos);
        bg.setFillColor(sf::Color(50, 54, 64));
        bg.setOutlineThickness(1.0f);
        bg.setOutlineColor(sf::Color(18, 20, 25));
        rw.draw(bg);

        // Color strip (top 18% of tile)
        if (col != Color::DEFAULT) {
            sf::RectangleShape strip({tileSz - 2.0f, stripH});
            strip.setPosition({pos.x + 1.0f, pos.y + 1.0f});
            strip.setFillColor(tileColor(col));
            rw.draw(strip);
        }
    }

    const float tokenR = tileSz * 0.18f;
    const float tokenSpacing = tokenR * 2.2f;

    // Group players by position
    std::array<std::vector<int>, TOTAL> playersAt{};
    for (int p = 0; p < static_cast<int>(state.players.size()); ++p) {
        const auto& pv = state.players[static_cast<size_t>(p)];
        if (pv.status != PlayerStatus::BANKRUPT) {
            int pos = pv.position;
            if (pos >= 0 && pos < TOTAL) {
                playersAt[static_cast<size_t>(pos)].push_back(p);
            }
        }
    }

    for (int i = 0; i < TOTAL; ++i) {
        const auto& ps = playersAt[static_cast<size_t>(i)];
        if (ps.empty()) continue;
        const sf::Vector2f base = tilePos(i);
        const float cx = base.x + tileSz * 0.5f;
        const float cy = base.y + tileSz * 0.65f;

        const float totalW = static_cast<float>(ps.size()) * tokenSpacing;
        float startX = cx - totalW * 0.5f + tokenSpacing * 0.5f;

        for (int playerIdx : ps) {
            sf::CircleShape token(tokenR);
            token.setFillColor(tokenColor(playerIdx));
            token.setOutlineThickness(1.5f);
            token.setOutlineColor(sf::Color(255, 255, 255, 160));
            token.setOrigin({tokenR, tokenR});
            token.setPosition({startX, cy});
            rw.draw(token);
            startX += tokenSpacing;
        }
    }

    rw.display();
#else
    (void)state;
#endif
}

void GUIView::showDiceResult(int d1, int d2, const string& playerName) {
    (void)d1;
    (void)d2;
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
    (void)entries;
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
    (void)playerName;
    (void)turnNum;
    (void)maxTurn;
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
