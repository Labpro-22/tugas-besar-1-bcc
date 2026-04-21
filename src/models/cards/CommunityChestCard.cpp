#include "models/cards/CommunityChestCard.hpp"

#include <utility>

CommunityChestCard::CommunityChestCard(std::string name, std::string description)
    : ActionCard(std::move(name), std::move(description)) {}

CommunityChestCard::~CommunityChestCard() = default;

std::string CommunityChestCard::getType() const {
    return "CommunityChestCard";
}
