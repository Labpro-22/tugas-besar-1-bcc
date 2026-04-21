#ifndef SHIELDEFFECT_HPP
#define SHIELDEFFECT_HPP
#include "models/effects/Effect.hpp"

class ShieldEffect : public Effect {
public:
    ShieldEffect();
    bool blockPayment() const override;
    std::string getDescription() const override;
    std::string getEffectType() const override;
};

#endif
