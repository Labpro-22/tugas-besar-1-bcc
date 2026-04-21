#pragma once

#include <string>

#include "models/cards/ChanceCard.hpp"

class ChanceMoveBackThreeCard : public ChanceCard {
    public:
        ChanceMoveBackThreeCard();
        ~ChanceMoveBackThreeCard() override;

        CardResult execute(Player& player, GameContext& context) override;
        std::string getId() const override;
};
