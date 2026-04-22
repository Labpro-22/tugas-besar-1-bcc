#ifndef FESTIVALMANAGER_HPP
#define FESTIVALMANAGER_HPP
#include <map>

class Player;
class PropertyTile;

class FestivalResult {
public:
    FestivalResult(bool applied, int multiplier, int turnsGranted);

    bool isApplied() const;
    int getMultiplier() const;
    int getTurnsGranted() const;

private:
    bool applied;
    int multiplier;
    int turnsGranted;
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
    class FestivalPropertyEffect {
    public:
        FestivalPropertyEffect();

        Player* getOwner() const;
        int getMultiplier() const;
        int getTurnsRemaining() const;
        int getTimesApplied() const;

        void setOwner(Player* owner);
        void setMultiplier(int value);
        void setTurnsRemaining(int value);
        void setTimesApplied(int value);

    private:
        Player* owner;
        int multiplier;
        int turnsRemaining;
        int timesApplied;
    };

    static int multiplierFor(int timesApplied);

    std::map<PropertyTile*, FestivalPropertyEffect> activeEffects;
};

#endif
