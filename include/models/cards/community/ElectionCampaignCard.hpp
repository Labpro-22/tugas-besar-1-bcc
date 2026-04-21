#pragma once

#include <string>

#include "models/cards/CommunityChestCard.hpp"

class ElectionCampaignCard : public CommunityChestCard {
    public:
        ElectionCampaignCard();
        ~ElectionCampaignCard() override;

        CardResult execute(Player& player, GameContext& context) override;
        std::string getId() const override;
};
