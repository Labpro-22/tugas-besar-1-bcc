#pragma once

#include <string>

#include "models/cards/SkillCard.hpp"

class ShieldCard : public SkillCard {
    public:
        explicit ShieldCard(int duration = 1);
        ~ShieldCard() override;

        CardResult activate(Player& player, GameContext& context) override;
        SkillCardType getCardType() const override;
        std::string getId() const override;
        int getDuration() const;

    private:
        int duration;
};
