#pragma once

#include <string>

#include "models/cards/SkillCard.hpp"

class LassoCard : public SkillCard {
    public:
        LassoCard();
        ~LassoCard() override;

        CardResult activate(Player& player, GameContext& context) override;
        SkillCardType getCardType() const override;
        std::string getId() const override;
};
