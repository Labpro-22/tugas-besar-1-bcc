#include "models/Player.hpp"
#include "models/effects/Effect.hpp"
#include "models/cards/SkillCard.hpp"
#include "controllers/PlayerController.hpp"
#include "tile/header/PropertyTile.hpp"
#include "tile/header/RailroadTile.hpp"
#include "tile/header/StreetTile.hpp"
#include "tile/header/UtilityTile.hpp"
#include "utils/Exceptions.hpp"
#include <algorithm>
#include <stdexcept>

Player::Player(const std::string& username, Money startingMoney, PlayerController* controller)
    : username(username), money(startingMoney), position(0),
      status(PlayerStatus::ACTIVE), controller(controller),
      jailTurnsRemaining(0), consecutiveDoubles(0),
      hasUsedSkillCardThisTurn(false), hasRolledDiceThisTurn(false),
      turnCount(0) {}

Player::~Player() = default;
Player::Player(Player&& other) noexcept = default;
Player& Player::operator=(Player&& other) noexcept = default;

const std::string& Player::getUsername() const {
    return username;
}

Money Player::getMoney() const {
    return money;
}

void Player::addMoney(const Money& amount) {
    money += amount;
}

void Player::deductMoney(const Money& amount) {
    money -= amount;
}

bool Player::canAfford(const Money& amount) const {
    return money >= amount;
}

int Player::getPosition() const {
    return position;
}

void Player::setPosition(int pos) {
    position = pos;
}

void Player::moveBy(int steps, int boardSize) {
    position = (position + steps % boardSize + boardSize) % boardSize;
}

PlayerStatus Player::getStatus() const {
    return status;
}

void Player::setStatus(PlayerStatus s) {
    status = s;
}

bool Player::isActive() const {
    return status == PlayerStatus::ACTIVE;
}

bool Player::isBankrupt() const {
    return status == PlayerStatus::BANKRUPT;
}

bool Player::isJailed() const {
    return status == PlayerStatus::JAILED;
}

const std::vector<PropertyTile*>& Player::getProperties() const {
    return properties;
}

void Player::addProperty(PropertyTile* tile) {
    properties.push_back(tile);
}

void Player::removeProperty(PropertyTile* tile) {
    properties.erase(std::remove(properties.begin(), properties.end(), tile), properties.end());
}

std::vector<StreetTile*> Player::getPropertiesByColor(Color color) const {
    std::vector<StreetTile*> result;
    for (PropertyTile* property : properties) {
        if (auto* street = dynamic_cast<StreetTile*>(property)) {
            if (street->getColor() == color) {
                result.push_back(street);
            }
        }
    }
    return result;
}

bool Player::hasMonopoly(Color color) const {
    const std::vector<StreetTile*> owned = getPropertiesByColor(color);
    if (owned.empty()) {
        return false;
    }
    for (const StreetTile* street : owned) {
        if (!street->isMonopolyComplete()) {
            return false;
        }
    }
    return true;
}

int Player::countRailroads() const {
    int count = 0;
    for (const PropertyTile* property : properties) {
        if (property && property->getType() == TileType::RAILROAD && !property->isMortgaged()) {
            ++count;
        }
    }
    return count;
}

int Player::countUtilities() const {
    int count = 0;
    for (const PropertyTile* property : properties) {
        if (property && property->getType() == TileType::UTILITY && !property->isMortgaged()) {
            ++count;
        }
    }
    return count;
}

const std::vector<SkillCard*>& Player::getSkillCards() const {
    skillCardView.clear();
    for (const auto& card : skillCards) {
        skillCardView.push_back(card.get());
    }
    return skillCardView;
}

void Player::addSkillCard(SkillCard* card) {
    if (!card) {
        throw std::invalid_argument("Cannot add null skill card.");
    }
    if (static_cast<int>(skillCards.size()) >= MAX_SKILL_CARDS)
        throw CardSlotFullException(MAX_SKILL_CARDS);
    skillCards.emplace_back(card);
}

SkillCard* Player::removeSkillCard(int index) {
    if (index < 0 || index >= static_cast<int>(skillCards.size())) {
        return nullptr;
    }
    SkillCard* c = skillCards[index].release();
    skillCards.erase(skillCards.begin() + index);
    return c;
}

const std::vector<Effect*>& Player::getActiveEffects() const {
    activeEffectView.clear();
    for (const auto& effect : activeEffects) {
        activeEffectView.push_back(effect.get());
    }
    return activeEffectView;
}

void Player::addEffect(Effect* effect) {
    if (!effect) {
        return;
    }
    effect->onStart(*this);
    activeEffects.emplace_back(effect);
}

void Player::tickEffects() {
    for (auto& e : activeEffects)
        if (e) e->tick();

    for (auto it = activeEffects.begin(); it != activeEffects.end(); ) {
        if (*it && (*it)->isExpired()) {
            (*it)->onEnd(*this);
            it = activeEffects.erase(it);
        } else {
            ++it;
        }
    }
}

Money Player::applyOutgoingModifiers(const Money& amount) const {
    Money result = amount;
    for (const auto& e : activeEffects)
        if (e) result = e->modifyOutgoingPayment(result);
    return result;
}

Money Player::applyIncomingModifiers(const Money& amount, PropertyTile* prop) const {
    Money result = amount;
    for (const auto& e : activeEffects)
        if (e) result = e->modifyRent(result, prop);
    return result;
}

bool Player::isPaymentBlocked() const {
    for (const auto& e : activeEffects)
        if (e && e->blockPayment()) {
            return true;
        }
    return false;
}

PlayerController* Player::getController() const {
    return controller;
}

Money Player::getTotalWealth() const {
    Money total = money;
    for (const PropertyTile* property : properties) {
        if (!property) {
            continue;
        }
        total += property->getPrice();
        if (auto* street = dynamic_cast<const StreetTile*>(property)) {
            const int level = street->getBuildingLevel();
            const int houses = std::min(level, 4);
            total += Money(street->getHouseCost().getAmount() * houses);
            if (street->hasHotel()) {
                total += street->getHotelCost();
            }
        }
    }
    return total;
}

bool Player::getHasUsedSkillCardThisTurn() const {
    return hasUsedSkillCardThisTurn;
}

bool Player::getHasRolledDiceThisTurn() const {
    return hasRolledDiceThisTurn;
}

void Player::setHasUsedSkillCardThisTurn(bool v) {
    hasUsedSkillCardThisTurn = v;
}

void Player::setHasRolledDiceThisTurn(bool v) {
    hasRolledDiceThisTurn = v;
}

void Player::resetTurnFlags() {
    hasUsedSkillCardThisTurn = false;
    hasRolledDiceThisTurn = false;
}

void Player::clearSkillCardsAndEffects() {
    skillCards.clear();
    for (auto& effect : activeEffects) {
        if (effect) {
            effect->onEnd(*this);
        }
    }
    activeEffects.clear();
    properties.clear();
}

int Player::getJailTurnsRemaining() const {
    return jailTurnsRemaining;
}

void Player::incrementJailTurn() {
    ++jailTurnsRemaining;
}

void Player::resetJailTurns() {
    jailTurnsRemaining = 0;
}

int Player::getConsecutiveDoubles() const {
    return consecutiveDoubles;
}

void Player::setConsecutiveDoubles(int n) {
    consecutiveDoubles = n;
}

int Player::getTurnCount() const {
    return turnCount;
}

void Player::incrementTurnCount() {
    ++turnCount;
}

bool Player::operator<(const Player& other) const {
    return money < other.money;
}

bool Player::operator>(const Player& other) const {
    return money > other.money;
}
