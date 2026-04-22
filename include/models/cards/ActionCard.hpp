#pragma once

#include <string>

#include "models/cards/Card.hpp"
#include "models/cards/CardResult.hpp"

class Player;
class GameContext;

class ActionCard : public Card {
    public:
        ActionCard(std::string name, std::string description);
        ~ActionCard() override;

        virtual CardResult execute(Player& player, GameContext& context) = 0;
        std::string getType() const override;
};
