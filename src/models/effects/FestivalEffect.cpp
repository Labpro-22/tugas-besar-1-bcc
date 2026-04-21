#include "models/effects/FestivalEffect.hpp"

FestivalRentEffect::FestivalRentEffect(int multiplier, int turns) : Effect(turns), multiplier(multiplier) {}

Money FestivalRentEffect::modifyRent(const Money& amount, PropertyTile*) const {
    return Money(amount.getAmount() * multiplier);
}

std::string FestivalRentEffect::getDescription() const {
    return "Festival x" + std::to_string(multiplier) + " (" + std::to_string(remainingTurns) + " turns left)";
}

std::string FestivalRentEffect::getEffectType() const {
    return "Festival";   
}

int FestivalRentEffect::getMultiplier() const { 
    return multiplier; 
}
