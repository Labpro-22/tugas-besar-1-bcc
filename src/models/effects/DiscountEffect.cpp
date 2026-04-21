#include "models/effects/DiscountEffect.hpp"

DiscountEffect::DiscountEffect(int percent, int turns) : Effect(turns), percent(percent) {}

Money DiscountEffect::modifyOutgoingPayment(const Money& amount) const {
    return Money(amount.getAmount() * (100 - percent) / 100);
}

std::string DiscountEffect::getDescription() const {
    return "Discount " + std::to_string(percent) + "% (" + std::to_string(remainingTurns) + " turns left)";
}

std::string DiscountEffect::getEffectType() const { return "Discount"; }
