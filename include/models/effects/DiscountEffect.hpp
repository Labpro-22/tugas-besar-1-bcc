#ifndef DISCOUNTEFFECT_HPP
#define DISCOUNTEFFECT_HPP
#include "models/effects/Effect.hpp"

class DiscountEffect : public Effect {
public:
    DiscountEffect(int percent, int turns);
    Money modifyOutgoingPayment(const Money& amount) const override;
    std::string getDescription() const override;
    std::string getEffectType() const override;

private:
    int percent;
};

#endif
