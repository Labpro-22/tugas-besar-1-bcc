#pragma once

#include <string>

#include "models/cards/SkillCard.hpp"

class MoveCard : public SkillCard {
    public:
        explicit MoveCard(int steps = 1);
        ~MoveCard() override;

        CardResult activate(Player& player, GameContext& context) override;
        SkillCardType getCardType() const override;
        std::string getSaveValue() const override;
        std::string getId() const override;
        int getSteps() const;

    private:
        int steps;
};
