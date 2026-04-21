#pragma once

#include <string>

#include "models/cards/CommunityChestCard.hpp"

class DoctorFeeCard : public CommunityChestCard {
    public:
        DoctorFeeCard();
        ~DoctorFeeCard() override;

        CardResult execute(Player& player, GameContext& context) override;
        std::string getId() const override;
};
