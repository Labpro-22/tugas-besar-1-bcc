#include "controllers/ComputerController.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

namespace {
    string lower(string value) {
        transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(tolower(ch)); });
        return value;
    }

    string upper(string value) {
        transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(toupper(ch)); });
        return value;
    }

    int maxRent(const PropertyInfo& info) {
        int rent = 0;
        for (int value : info.rentTable) {
            rent = max(rent, value);
        }
        return rent;
    }

    int normalizedDistance(int from, int to, int boardSize) {
        if (boardSize <= 0) {
            return 0;
        }
        return (to - from + boardSize) % boardSize;
    }
}

ComputerController::ComputerController()
    : ComputerController("") {}

ComputerController::ComputerController(const string& playerName)
    : playerName_(playerName),
      lastState_(),
      lastTurn_(-1),
      lastActivePlayerIndex_(-1),
      lastActivePlayerName_(""),
      attemptedSkillThisTurn_(false),
      attemptedJailFineThisTurn_(false) {}

string ComputerController::chooseCommand(const GameStateView& state) {
    refreshTurnMemory(state);

    const PlayerView* active = activePlayerView(state);
    if (!active) {
        return "selesai";
    }

    if (active->status == PlayerStatus::BANKRUPT) {
        return "selesai";
    }

    if (!state.getHasRolledDice()) {
        if (active->status == PlayerStatus::JAILED) {
            if (!attemptedJailFineThisTurn_ && shouldPayJailFine(*active)) {
                attemptedJailFineThisTurn_ = true;
                return "bayar_denda";
            }
            return "lempar dadu";
        }

        if (shouldTrySkillCard(*active, state)) {
            attemptedSkillThisTurn_ = true;
            return "gunakan kemampuan 1";
        }

        return "lempar dadu";
    }

    return "selesai";
}

bool ComputerController::decideBuyProperty(const PropertyInfo& info, Money money) {
    const int cash = money.getAmount();
    const int price = info.purchasePrice.getAmount();

    if (info.status != PropertyStatus::BANK && !info.ownerName.empty()) {
        return false;
    }
    if (price <= 0) {
        return true;
    }
    if (cash < price) {
        return false;
    }

    const int score = propertyScore(info);
    int reserve = cashReserve(cash);
    if (score >= 85) {
        reserve /= 2;
    } else if (score >= 70) {
        reserve = reserve * 2 / 3;
    }

    const int cashAfterPurchase = cash - price;
    if (cashAfterPurchase >= reserve) {
        return true;
    }

    const int minimumEmergencyCash = max(25, cash / 20);
    if (score >= 85 && cashAfterPurchase >= minimumEmergencyCash) {
        return true;
    }

    return price <= cash / 3 && cashAfterPurchase >= 0;
}

AuctionDecision ComputerController::decideAuction(int currentBid, Money money) {
    const int cash = money.getAmount();
    const int minimumBid = max(0, currentBid + 1);
    if (cash <= minimumBid) {
        return AuctionDecision{AuctionAction::PASS, 0};
    }

    int budget = cash - cashReserve(cash);
    budget = min(budget, cash * 45 / 100);
    if (cash <= 2000) {
        budget = min(budget, 420);
    }

    if (budget < minimumBid) {
        return AuctionDecision{AuctionAction::PASS, 0};
    }

    const int step = max(10, cash / 50);
    int bid = currentBid < 0 ? min(budget, max(50, step * 3)) : min(budget, currentBid + step);
    bid = max(bid, minimumBid);

    if (bid > budget || bid > cash) {
        return AuctionDecision{AuctionAction::PASS, 0};
    }

    return AuctionDecision{AuctionAction::BID, bid};
}

TaxChoice ComputerController::decideTax(int flatAmount, int percentAmount) {
    return flatAmount <= percentAmount ? TaxChoice::FLAT : TaxChoice::PERCENTAGE;
}

int ComputerController::decideSkillCard(const vector<CardInfo>& cards) {
    if (cards.empty()) {
        return 0;
    }

    int bestIndex = 0;
    int bestScore = numeric_limits<int>::min();
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const int score = cardScore(cards[i]);
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    return bestIndex + 1;
}

string ComputerController::decideFestivalProperty(const vector<PropertyInfo>& props) {
    if (props.empty()) {
        return "";
    }

    const PropertyInfo* best = nullptr;
    int bestScore = numeric_limits<int>::min();
    for (const PropertyInfo& prop : props) {
        if (prop.code.empty() || prop.status == PropertyStatus::MORTGAGED) {
            continue;
        }

        int score = propertyScore(prop);
        score += prop.buildingLevel * 120;
        score += maxRent(prop);
        score += max(0, prop.festivalMultiplier - 1) * 40;

        if (!best || score > bestScore) {
            best = &prop;
            bestScore = score;
        }
    }

    return best ? best->code : props.front().code;
}

int ComputerController::decideBuild(const BuildMenuState& state) {
    int bestIndex = -1;
    int bestScore = numeric_limits<int>::min();
    const int cash = state.playerMoney.getAmount();
    const int reserve = cashReserve(cash);

    for (int i = 0; i < static_cast<int>(state.groups.size()); ++i) {
        const ColorGroupBuildOption& group = state.groups[i];
        int score = 0;
        bool hasAffordableBuild = false;

        for (const TileBuildOption& tile : group.tiles) {
            if (!tile.canBuild || tile.buildCost <= 0 || cash - tile.buildCost < reserve) {
                continue;
            }

            hasAffordableBuild = true;
            score += 1000 / max(1, tile.buildCost);
            score += (5 - min(tile.currentLevel, 5)) * 15;
            score += propertyCodeScore(tile.code);
        }

        if (hasAffordableBuild && score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    return bestIndex >= 0 ? bestIndex + 1 : 0;
}

int ComputerController::decideLiquidation(const LiquidationState& state) {
    if (state.options.empty()) {
        return 0;
    }

    const int needed = max(0, state.obligation.getAmount() - state.currentBalance.getAmount());
    if (needed <= 0) {
        return 0;
    }

    int bestCoverIndex = -1;
    int bestCoverScore = numeric_limits<int>::max();
    int bestFallbackIndex = 0;
    int bestFallbackValue = numeric_limits<int>::min();

    for (int i = 0; i < static_cast<int>(state.options.size()); ++i) {
        const LiquidationOption& option = state.options[i];
        const int value = option.value.getAmount();
        const int irreversiblePenalty = option.type == LiquidationType::SELL ? 80 : 0;

        if (value >= needed) {
            const int waste = value - needed;
            const int score = waste + irreversiblePenalty;
            if (score < bestCoverScore) {
                bestCoverScore = score;
                bestCoverIndex = i;
            }
        }

        const int fallbackScore = value - irreversiblePenalty;
        if (fallbackScore > bestFallbackValue) {
            bestFallbackValue = fallbackScore;
            bestFallbackIndex = i;
        }
    }

    const int chosen = bestCoverIndex >= 0 ? bestCoverIndex : bestFallbackIndex;
    return liquidationReturnIndex(state, chosen);
}

int ComputerController::decideDropCard(const vector<CardInfo>& cards) {
    if (cards.empty()) {
        return 0;
    }

    int worstIndex = 0;
    int worstScore = numeric_limits<int>::max();
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const int score = cardScore(cards[i]);
        if (score < worstScore) {
            worstScore = score;
            worstIndex = i;
        }
    }
    return worstIndex + 1;
}

bool ComputerController::decideJailPay() {
    return true;
}

string ComputerController::decideTeleportTarget() {
    return bestTeleportTarget();
}

string ComputerController::decideLassoTarget(const vector<string>& players) {
    if (players.empty()) {
        return "";
    }

    const PlayerView* active = activePlayerView(lastState_);
    const int boardSize = !lastState_.tiles.empty() ? static_cast<int>(lastState_.tiles.size()) : 40;
    string bestName = players.front();
    int bestScore = numeric_limits<int>::min();

    for (const string& name : players) {
        auto it = find_if(lastState_.players.begin(), lastState_.players.end(),
                          [&](const PlayerView& view) { return view.username == name; });
        if (it == lastState_.players.end()) {
            continue;
        }

        int score = it->money.getAmount();
        if (active) {
            const int distance = normalizedDistance(active->position, it->position, boardSize);
            if (distance <= 0) {
                continue;
            }
            score += (boardSize - distance) * 5;
        }

        if (score > bestScore) {
            bestScore = score;
            bestName = name;
        }
    }

    return bestName;
}

string ComputerController::decideDemolitionTarget(const vector<PropertyInfo>& properties) {
    if (properties.empty()) {
        return "";
    }

    const string activeName = lastState_.getActivePlayerName();
    const PropertyInfo* best = nullptr;
    int bestScore = numeric_limits<int>::min();

    for (const PropertyInfo& property : properties) {
        if (property.code.empty() || property.ownerName == activeName) {
            continue;
        }

        int score = propertyScore(property);
        score += property.buildingLevel * 500;
        score += property.purchasePrice.getAmount() / 2;
        score += maxRent(property);
        score += max(0, property.festivalMultiplier - 1) * 100;
        if (property.status == PropertyStatus::MORTGAGED) {
            score -= 100;
        }

        if (!best || score > bestScore) {
            best = &property;
            bestScore = score;
        }
    }

    return best ? best->code : properties.front().code;
}

void ComputerController::refreshTurnMemory(const GameStateView& state) {
    const bool changedTurn =
        state.getCurrentTurn() != lastTurn_ ||
        state.getActivePlayerIndex() != lastActivePlayerIndex_ ||
        state.getActivePlayerName() != lastActivePlayerName_;

    lastState_ = state;

    if (!changedTurn) {
        return;
    }

    lastTurn_ = state.getCurrentTurn();
    lastActivePlayerIndex_ = state.getActivePlayerIndex();
    lastActivePlayerName_ = state.getActivePlayerName();
    attemptedSkillThisTurn_ = false;
    attemptedJailFineThisTurn_ = false;
}

const PlayerView* ComputerController::activePlayerView(const GameStateView& state) const {
    const string& activeName = state.getActivePlayerName();
    if (!activeName.empty()) {
        for (const PlayerView& player : state.getPlayers()) {
            if (player.username == activeName) {
                return &player;
            }
        }
    }

    const int index = state.getActivePlayerIndex();
    if (index >= 0 && index < static_cast<int>(state.getPlayers().size())) {
        return &state.getPlayers()[index];
    }

    if (!playerName_.empty()) {
        for (const PlayerView& player : state.getPlayers()) {
            if (player.username == playerName_) {
                return &player;
            }
        }
    }

    return nullptr;
}

bool ComputerController::shouldTrySkillCard(const PlayerView& player, const GameStateView& state) const {
    return !attemptedSkillThisTurn_ &&
           !state.getHasUsedSkillCard() &&
           player.status == PlayerStatus::ACTIVE &&
           player.skillCardCount > 0;
}

bool ComputerController::shouldPayJailFine(const PlayerView& player) const {
    const int cash = player.money.getAmount();
    if (cash <= 0) {
        return false;
    }
    return cash >= max(100, cashReserve(cash) + 50);
}

int ComputerController::cashReserve(int cash) const {
    if (cash <= 0) {
        return 0;
    }

    const int proportionalReserve = cash / 5;
    if (cash <= 300) {
        return min(75, max(25, proportionalReserve));
    }
    if (cash <= 1200) {
        return max(100, proportionalReserve);
    }
    return max(200, proportionalReserve);
}

int ComputerController::propertyScore(const PropertyInfo& info) const {
    int score = propertyCodeScore(info.code);

    switch (info.type) {
        case TileType::STREET:
            score += 15;
            break;
        case TileType::RAILROAD:
            score += 20;
            break;
        case TileType::UTILITY:
            score += 5;
            break;
        default:
            break;
    }

    switch (info.colorGroup) {
        case Color::ORANGE:
            score += 30;
            break;
        case Color::RED:
            score += 28;
            break;
        case Color::LIGHT_BLUE:
            score += 24;
            break;
        case Color::YELLOW:
            score += 22;
            break;
        case Color::PINK:
            score += 18;
            break;
        case Color::GREEN:
            score += 17;
            break;
        case Color::DARK_BLUE:
            score += 15;
            break;
        case Color::BROWN:
            score += 14;
            break;
        default:
            break;
    }

    const int price = info.purchasePrice.getAmount();
    const int bestRent = maxRent(info);
    if (bestRent > 0) {
        score += min(35, bestRent / 40);
        if (price > 0) {
            score += min(25, bestRent * 10 / price);
        }
    }

    score += info.buildingLevel * 20;
    if (info.status == PropertyStatus::MORTGAGED) {
        score -= 35;
    }
    return score;
}

int ComputerController::propertyCodeScore(const string& code) const {
    const string normalized = upper(code);

    if (normalized == "MAL" || normalized == "SMG" || normalized == "SBY") return 95;
    if (normalized == "BGR" || normalized == "DPK" || normalized == "BKS") return 85;
    if (normalized == "MKS" || normalized == "BLP" || normalized == "MND") return 82;
    if (normalized == "PLB" || normalized == "PKB" || normalized == "MED") return 78;
    if (normalized == "MGL" || normalized == "SOL" || normalized == "YOG") return 72;
    if (normalized == "BDG" || normalized == "DEN" || normalized == "MTR") return 70;
    if (normalized == "JKT" || normalized == "IKN") return 68;
    if (normalized == "GRT" || normalized == "TSK") return 60;
    if (normalized == "GBR" || normalized == "STB" || normalized == "TUG" || normalized == "GUB") return 55;
    if (normalized == "PLN" || normalized == "PAM") return 40;
    return 50;
}

int ComputerController::cardScore(const CardInfo& card) const {
    const string text = lower(card.name + " " + card.description + " " + card.type);
    int score = 35;

    if (text.find("shield") != string::npos || text.find("kebal") != string::npos) {
        score = 95;
    } else if (text.find("discount") != string::npos || text.find("diskon") != string::npos) {
        score = 85;
    } else if (text.find("demolition") != string::npos || text.find("hancur") != string::npos) {
        score = 80;
    } else if (text.find("teleport") != string::npos || text.find("pindah") != string::npos) {
        score = 75;
    } else if (text.find("move") != string::npos || text.find("maju") != string::npos) {
        score = 65;
    } else if (text.find("lasso") != string::npos || text.find("tarik") != string::npos) {
        score = 55;
    }

    score += min(15, max(0, card.value));
    score += min(10, max(0, card.durationLeft));
    return score;
}

int ComputerController::liquidationReturnIndex(const LiquidationState& state, int optionPosition) const {
    if (optionPosition < 0 || optionPosition >= static_cast<int>(state.options.size())) {
        return 0;
    }

    const int explicitIndex = state.options[optionPosition].index;
    return explicitIndex > 0 ? explicitIndex : optionPosition + 1;
}

string ComputerController::bestTeleportTarget() const {
    const string activeName = lastState_.getActivePlayerName();
    const PropertyView* best = nullptr;
    int bestScore = numeric_limits<int>::min();

    for (const PropertyView& property : lastState_.getProperties()) {
        const bool bankOwned = property.status == PropertyStatus::BANK || property.ownerName.empty();
        if (!bankOwned || property.code.empty()) {
            continue;
        }

        int score = propertyCodeScore(property.code);
        score += property.buildingLevel * 20;
        if (property.ownerName == activeName) {
            score -= 100;
        }

        if (!best || score > bestScore) {
            best = &property;
            bestScore = score;
        }
    }

    return best ? best->code : fallbackSafeTileCode();
}

string ComputerController::fallbackSafeTileCode() const {
    for (const TileView& tile : lastState_.tiles) {
        if (tile.type == TileType::GO) {
            return tile.code;
        }
    }
    return "GO";
}
