#pragma once

#include <string>

#include "models/cards/SkillCard.hpp"

class DemolitionCard : public SkillCard {
    public:
        DemolitionCard();
        ~DemolitionCard() override;

        CardResult activate(Player& player, GameContext& context) override;
        SkillCardType getCardType() const override;
        std::string getId() const override;
};
