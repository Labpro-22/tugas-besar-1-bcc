#include "core/FestivalEffectSnapshot.hpp"

FestivalEffectSnapshot::FestivalEffectSnapshot(PropertyTile* property, Player* owner, int multiplier, int turnsRemaining,
	int timesApplied)
	: property(property), owner(owner), multiplier(multiplier), turnsRemaining(turnsRemaining), timesApplied(timesApplied) {
}

PropertyTile* FestivalEffectSnapshot::getProperty() const {
	return property;
}

Player* FestivalEffectSnapshot::getOwner() const {
	return owner;
}

int FestivalEffectSnapshot::getMultiplier() const {
	return multiplier;
}

int FestivalEffectSnapshot::getTurnsRemaining() const {
	return turnsRemaining;
}

int FestivalEffectSnapshot::getTimesApplied() const {
	return timesApplied;
}
