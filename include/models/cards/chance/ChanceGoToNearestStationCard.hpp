#pragma once

#include <string>

#include "models/cards/ChanceCard.hpp"

class ChanceGoToNearestStationCard : public ChanceCard {
    public:
        ChanceGoToNearestStationCard();
        ~ChanceGoToNearestStationCard() override;

        CardResult execute(Player& player, GameContext& context) override;
        std::string getId() const override;
};
