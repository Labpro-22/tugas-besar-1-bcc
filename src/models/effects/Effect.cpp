#include "models/effects/Effect.hpp"

Effect::Effect(int remainingTurns) : remainingTurns(remainingTurns) {}

void Effect::onStart(Player&) {}
void Effect::onEnd(Player&) {}

Money Effect::modifyOutgoingPayment(const Money& amount) const { return amount; }
Money Effect::modifyRent(const Money& amount, PropertyTile*) const { return amount; }
bool Effect::blockPayment() const { return false; }

void Effect::tick() { if (remainingTurns > 0) --remainingTurns; }
bool Effect::isExpired() const { return remainingTurns <= 0; }
int Effect::getRemainingTurns() const { return remainingTurns; }

std::string Effect::getDescription() const { return "Effect"; }
std::string Effect::getEffectType() const { return "Effect"; }
