#pragma once

#include <string>

#include "models/cards/Card.hpp"
#include "models/cards/CardResult.hpp"
#include "utils/Enums.hpp"

class Player;
struct GameContext;

class SkillCard : public Card {
    public:
        SkillCard(std::string name, std::string description);
        ~SkillCard() override;

        virtual CardResult activate(Player& player, GameContext& context) = 0;
        virtual SkillCardType getCardType() const = 0;
        virtual std::string getSaveValue() const;
        virtual std::string getSaveDuration() const;
        std::string getType() const override;
};
