#pragma once

#include <string>

#include "models/cards/SkillCard.hpp"

class DiscountCard : public SkillCard {
    public:
        DiscountCard(int percentage = 30, int duration = 1);
        ~DiscountCard() override;

        CardResult activate(Player& player, GameContext& context) override;
        SkillCardType getCardType() const override;
        std::string getSaveValue() const override;
        std::string getSaveDuration() const override;
        std::string getId() const override;
        int getPercentage() const;
        int getDuration() const;

    private:
        int percentage;
        int duration;
};
