#pragma once

#include <string>

#include "models/cards/ActionCard.hpp"

class ChanceCard : public ActionCard {
    public:
        ChanceCard(std::string name, std::string description);
        ~ChanceCard() override;

        std::string getType() const override;
};
