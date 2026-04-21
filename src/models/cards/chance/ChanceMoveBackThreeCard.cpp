#include "models/cards/chance/ChanceMoveBackThreeCard.hpp"

ChanceMoveBackThreeCard::ChanceMoveBackThreeCard()
    : ChanceCard("Mundur 3 petak", "Mundur tiga petak dari posisi saat ini.") {}

ChanceMoveBackThreeCard::~ChanceMoveBackThreeCard() = default;

CardResult ChanceMoveBackThreeCard::execute(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::MOVE_RELATIVE,
        "Mundur 3 petak."
    );
    result.value = 3;
    result.movementOffset = -3;
    result.movedPlayer = true;
    result.resolveLanding = true;
    return result;
}

std::string ChanceMoveBackThreeCard::getId() const {
    return "ChanceMoveBackThreeCard";
}
