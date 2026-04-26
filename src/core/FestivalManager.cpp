#include "core/FestivalManager.hpp"
#include "models/Player.hpp"

FestivalResult::FestivalResult(bool applied, int multiplier, int turnsGranted)
    : applied(applied), multiplier(multiplier), turnsGranted(turnsGranted) {}

bool FestivalResult::isApplied() const {
    return applied;
}

int FestivalResult::getMultiplier() const {
    return multiplier;
}

int FestivalResult::getTurnsGranted() const {
    return turnsGranted;
}

FestivalManager::FestivalPropertyEffect::FestivalPropertyEffect()
    : owner(nullptr), multiplier(1), turnsRemaining(0), timesApplied(0) {}

Player* FestivalManager::FestivalPropertyEffect::getOwner() const {
    return owner;
}

int FestivalManager::FestivalPropertyEffect::getMultiplier() const {
    return multiplier;
}

int FestivalManager::FestivalPropertyEffect::getTurnsRemaining() const {
    return turnsRemaining;
}

int FestivalManager::FestivalPropertyEffect::getTimesApplied() const {
    return timesApplied;
}

void FestivalManager::FestivalPropertyEffect::setOwner(Player* v) {
    owner = v;
}

void FestivalManager::FestivalPropertyEffect::setMultiplier(int v) {
    multiplier = v;
}

void FestivalManager::FestivalPropertyEffect::setTurnsRemaining(int v) {
    turnsRemaining = v;
}

void FestivalManager::FestivalPropertyEffect::setTimesApplied(int v) {
    timesApplied = v;
}

FestivalManager::FestivalManager() = default;


int FestivalManager::multiplierFor(int timesApplied) {
    switch (timesApplied) {
        case 1:  return 2;
        case 2:  return 4;
        case 3:  return 8;
        default: return 1;
    }
}

FestivalResult FestivalManager::applyFestival(Player& player, PropertyTile& property) {
    auto it = activeEffects.find(&property);

    if (it != activeEffects.end()) {
        FestivalPropertyEffect& fe = it->second;

        if (fe.getTimesApplied() >= 3) {
            fe.setTurnsRemaining(3);
            return FestivalResult(true, fe.getMultiplier(), 3);
        }

        fe.setTimesApplied(fe.getTimesApplied() + 1);
        fe.setMultiplier(multiplierFor(fe.getTimesApplied()));
        fe.setTurnsRemaining(3);

        return FestivalResult(true, fe.getMultiplier(), 3);
    }

    FestivalPropertyEffect fe;
    fe.setOwner(&player);
    fe.setTimesApplied(1);
    fe.setMultiplier(multiplierFor(1));
    fe.setTurnsRemaining(3);
    activeEffects[&property] = fe;

    return FestivalResult(true, fe.getMultiplier(), 3);
}


void FestivalManager::tickPlayerEffects(Player& player) {
    player.tickEffects();

    for (auto it = activeEffects.begin(); it != activeEffects.end(); ) {
        if (it->second.getOwner() == &player) {
            it->second.setTurnsRemaining(it->second.getTurnsRemaining() - 1);
            if (it->second.getTurnsRemaining() <= 0)
                it = activeEffects.erase(it);
            else
                ++it;
        } else {
            ++it;
        }
    }
}

int FestivalManager::getMultiplier(PropertyTile* property) const {
    auto it = activeEffects.find(property);
    return it != activeEffects.end() ? it->second.getMultiplier() : 1;
}

int FestivalManager::getDuration(PropertyTile* property) const {
    auto it = activeEffects.find(property);
    return it != activeEffects.end() ? it->second.getTurnsRemaining() : 0;
}

bool FestivalManager::hasActiveEffect(PropertyTile* property) const {
    return activeEffects.count(property) > 0;
}

std::vector<FestivalEffectSnapshot> FestivalManager::getActiveEffectsSnapshot() const {
    std::vector<FestivalEffectSnapshot> out;
    out.reserve(activeEffects.size());
    for (const auto& entry : activeEffects) {
        const FestivalPropertyEffect& fe = entry.second;
        out.emplace_back(entry.first, fe.getOwner(), fe.getMultiplier(), fe.getTurnsRemaining(), fe.getTimesApplied());
    }
    return out;
}

void FestivalManager::restoreEffect(PropertyTile* prop, Player* owner, int mult, int turnsRemaining, int timesApplied) {
    if (!prop || !owner || mult <= 0 || turnsRemaining <= 0) {
        return;
    }
    FestivalPropertyEffect fe;
    fe.setOwner(owner);
    fe.setMultiplier(mult);
    fe.setTurnsRemaining(turnsRemaining);
    fe.setTimesApplied(timesApplied);
    activeEffects[prop] = fe;
}

void FestivalManager::clearAllEffects() {
    activeEffects.clear();
}

void FestivalManager::removeEffectsForProperty(PropertyTile* prop) {
    if (!prop) {
        return;
    }
    activeEffects.erase(prop);
}
