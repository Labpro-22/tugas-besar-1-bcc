#include "models/cards/ChanceCard.hpp"

#include <utility>

ChanceCard::ChanceCard(std::string name, std::string description)
    : ActionCard(std::move(name), std::move(description)) {}

ChanceCard::~ChanceCard() = default;

std::string ChanceCard::getType() const {
    return "ChanceCard";
}
