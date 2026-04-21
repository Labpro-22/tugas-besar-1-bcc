#pragma once

#include <string>

#include "models/cards/SkillCard.hpp"

class TeleportCard : public SkillCard {
    public:
        TeleportCard();
        ~TeleportCard() override;

        CardResult activate(Player& player, GameContext& context) override;
        SkillCardType getCardType() const override;
        std::string getId() const override;
};
