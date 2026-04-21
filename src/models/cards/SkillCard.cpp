#include "models/cards/SkillCard.hpp"

#include <utility>

SkillCard::SkillCard(std::string name, std::string description)
    : Card(std::move(name), std::move(description)) {}

SkillCard::~SkillCard() = default;

std::string SkillCard::getSaveValue() const {
    return "";
}

std::string SkillCard::getSaveDuration() const {
    return "";
}

std::string SkillCard::getType() const {
    return "SkillCard";
}
