#pragma once

#include <string>

#include "models/cards/ChanceCard.hpp"

class ChanceGoToJailCard : public ChanceCard {
    public:
        ChanceGoToJailCard();
        ~ChanceGoToJailCard() override;

        CardResult execute(Player& player, GameContext& context) override;
        std::string getId() const override;
};
