#include "models/cards/Effect.hpp"

#include <stdexcept>

Effect::Effect(int remainingTurns) : remainingTurns(remainingTurns) {
    if (remainingTurns < 0) {
        throw std::invalid_argument("Effect duration cannot be negative.");
    }
}

Effect::~Effect() = default;

void Effect::onStart() {}

void Effect::onEnd() {
    if (remainingTurns > 0) {
        --remainingTurns;
    }
}

Money Effect::modifyOutgoingPayment(const Money& amount) const {
    return amount;
}

Money Effect::modifyRent(const Money& amount, const PropertyTile*) const {
    return amount;
}

bool Effect::blockPayment() const {
    return false;
}

bool Effect::isExpired() const {
    return remainingTurns == 0;
}

int Effect::getRemainingTurns() const {
    return remainingTurns;
}

void Effect::setRemainingTurns(int turns) {
    if (turns < 0) {
        throw std::invalid_argument("Effect duration cannot be negative.");
    }
    remainingTurns = turns;
}

ShieldEffect::ShieldEffect(int remainingTurns) : Effect(remainingTurns) {}

bool ShieldEffect::blockPayment() const {
    return true;
}

std::string ShieldEffect::getDescription() const {
    return "Blocks harmful payments and penalties.";
}

std::string ShieldEffect::getEffectType() const {
    return "ShieldEffect";
}

DiscountEffect::DiscountEffect(int percentage, int remainingTurns)
    : Effect(remainingTurns),
      percentage(percentage) {
    if (percentage < 0 || percentage > 100) {
        throw std::invalid_argument("Discount percentage must be between 0 and 100.");
    }
}

Money DiscountEffect::modifyOutgoingPayment(const Money& amount) const {
    return Money(amount.getAmount() * (100 - percentage) / 100);
}

int DiscountEffect::getPercentage() const {
    return percentage;
}

std::string DiscountEffect::getDescription() const {
    return "Reduces outgoing payments by " + std::to_string(percentage) + "%.";
}

std::string DiscountEffect::getEffectType() const {
    return "DiscountEffect";
}
