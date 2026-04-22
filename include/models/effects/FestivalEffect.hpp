#ifndef FESTIVALEFFECT_HPP
#define FESTIVALEFFECT_HPP
#include "models/effects/Effect.hpp"

// Named FestivalRentEffect to avoid collision with global class FestivalEffect (utils/Types.hpp).
class FestivalRentEffect : public Effect {
public:
    FestivalRentEffect(int multiplier, int turns);
    Money modifyRent(const Money& amount, PropertyTile* prop) const override;
    std::string getDescription() const override;
    std::string getEffectType() const override;
    int getMultiplier() const;

private:
    int multiplier;
};

#endif
