#include "models/cards/ActionCard.hpp"

#include <utility>

ActionCard::ActionCard(std::string name, std::string description)
    : Card(std::move(name), std::move(description)) {}

ActionCard::~ActionCard() = default;

std::string ActionCard::getType() const {
    return "ActionCard";
}
