#ifndef FESTIVALMANAGER_HPP
#define FESTIVALMANAGER_HPP
#include <map>
#include "utils/Structs.hpp"

class Player;
class PropertyTile;

struct FestivalResult {
    bool applied;        // false if has 3 festival applications
    int  multiplier;     // 2/4/8
    int  turnsGranted;
};

class FestivalManager {
public:
    FestivalManager();

    FestivalResult applyFestival(Player& player, PropertyTile& property);
    void tickPlayerEffects(Player& player);

    int  getMultiplier(PropertyTile* property) const;
    int  getDuration(PropertyTile* property) const;
    bool hasActiveEffect(PropertyTile* property) const;

private:
    static int multiplierFor(int timesApplied);

    std::map<PropertyTile*, FestivalEffect> activeEffects;
};

#endif
