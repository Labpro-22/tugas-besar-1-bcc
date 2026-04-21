#include "models/cards/chance/ChanceGoToNearestStationCard.hpp"

ChanceGoToNearestStationCard::ChanceGoToNearestStationCard()
    : ChanceCard("Pergi ke stasiun terdekat", "Pindah ke stasiun terdekat dari posisi saat ini.") {}

ChanceGoToNearestStationCard::~ChanceGoToNearestStationCard() = default;

CardResult ChanceGoToNearestStationCard::execute(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::MOVE_TO_NEAREST_STATION,
        "Pergi ke stasiun terdekat."
    );
    result.movedPlayer = true;
    result.resolveLanding = true;
    return result;
}

std::string ChanceGoToNearestStationCard::getId() const {
    return "ChanceGoToNearestStationCard";
}
