#pragma once

#include <string>

#include "models/cards/ActionCard.hpp"

class CommunityChestCard : public ActionCard {
    public:
        CommunityChestCard(std::string name, std::string description);
        ~CommunityChestCard() override;

        std::string getType() const override;
};
