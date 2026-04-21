#include "models/cards/community/ElectionCampaignCard.hpp"

ElectionCampaignCard::ElectionCampaignCard()
    : CommunityChestCard("Biaya nyaleg", "Bayar M200 kepada setiap pemain.") {}

ElectionCampaignCard::~ElectionCampaignCard() = default;

CardResult ElectionCampaignCard::execute(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::PAY_EACH_PLAYER,
        "Anda mau nyaleg. Bayar M200 kepada setiap pemain."
    );
    result.amount = Money(200);
    result.value = 200;
    result.nextAction = LandingAction::BANKRUPT_CHECK;
    return result;
}

std::string ElectionCampaignCard::getId() const {
    return "ElectionCampaignCard";
}
