#pragma once

#include <string>

#include "models/cards/CommunityChestCard.hpp"

class BirthdayCard : public CommunityChestCard {
    public:
        BirthdayCard();
        ~BirthdayCard() override;

        CardResult execute(Player& player, GameContext& context) override;
        std::string getId() const override;
};
