#include "models/cards/skill/MoveCard.hpp"

#include <stdexcept>

MoveCard::MoveCard(int steps)
    : SkillCard("MoveCard", "Maju " + std::to_string(steps) + " petak."),
      steps(steps) {
    if (steps <= 0) {
        throw std::invalid_argument("MoveCard steps must be positive.");
    }
}

MoveCard::~MoveCard() = default;

CardResult MoveCard::activate(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::MOVE_RELATIVE,
        "MoveCard digunakan. Maju " + std::to_string(steps) + " petak."
    );
    result.value = steps;
    result.movementOffset = steps;
    result.movedPlayer = true;
    result.resolveLanding = true;
    return result;
}

SkillCardType MoveCard::getCardType() const {
    return SkillCardType::MOVE;
}

std::string MoveCard::getSaveValue() const {
    return std::to_string(steps);
}

std::string MoveCard::getId() const {
    return "MoveCard";
}

int MoveCard::getSteps() const {
    return steps;
}
