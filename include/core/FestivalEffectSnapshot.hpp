#ifndef CORE_FESTIVAL_EFFECT_SNAPSHOT_HPP
#define CORE_FESTIVAL_EFFECT_SNAPSHOT_HPP

class Player;
class PropertyTile;

class FestivalEffectSnapshot {
public:
	FestivalEffectSnapshot(PropertyTile* property, Player* owner, int multiplier, int turnsRemaining, int timesApplied);

	PropertyTile* getProperty() const;
	Player* getOwner() const;
	int getMultiplier() const;
	int getTurnsRemaining() const;
	int getTimesApplied() const;

private:
	PropertyTile* property;
	Player* owner;
	int multiplier;
	int turnsRemaining;
	int timesApplied;
};

#endif
