#ifndef EFFECT_HPP
#define EFFECT_HPP
#include <string>
#include "models/Money.hpp"

class Player;
class PropertyTile;

class Effect {
public:
    explicit Effect(int remainingTurns);
    virtual ~Effect() = default;

    virtual void onStart(Player& player);
    virtual void onEnd(Player& player);

    virtual Money modifyOutgoingPayment(const Money& amount) const;
    virtual Money modifyRent(const Money& amount, PropertyTile* prop) const;
    virtual bool blockPayment() const;

    void tick();
    bool isExpired() const;
    int getRemainingTurns() const;

    virtual std::string getDescription() const;
    virtual std::string getEffectType() const;

protected:
    int remainingTurns;
};

#endif
