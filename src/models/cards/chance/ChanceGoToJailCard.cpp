#include "models/cards/chance/ChanceGoToJailCard.hpp"

ChanceGoToJailCard::ChanceGoToJailCard()
    : ChanceCard("Masuk Penjara", "Pindahkan pemain langsung ke penjara.") {}

ChanceGoToJailCard::~ChanceGoToJailCard() = default;

CardResult ChanceGoToJailCard::execute(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::SEND_TO_JAIL,
        "Masuk Penjara."
    );
    result.nextAction = LandingAction::GO_TO_JAIL;
    result.movedPlayer = true;
    result.resolveLanding = false;
    return result;
}

std::string ChanceGoToJailCard::getId() const {
    return "ChanceGoToJailCard";
}
