#include "models/effects/ShieldEffect.hpp"

ShieldEffect::ShieldEffect() : Effect(1) {}
bool ShieldEffect::blockPayment() const { 
    return true;
}

std::string ShieldEffect::getDescription() const { 
    return "Shield (blocks next payment, " + std::to_string(remainingTurns) + " turn left)";
}

std::string ShieldEffect::getEffectType() const { 
    return "Shield";
}