#include "models/cards/community/DoctorFeeCard.hpp"

DoctorFeeCard::DoctorFeeCard()
    : CommunityChestCard("Biaya dokter", "Bayar M700 ke Bank.") {}

DoctorFeeCard::~DoctorFeeCard() = default;

CardResult DoctorFeeCard::execute(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::PAY_BANK,
        "Biaya dokter. Bayar M700."
    );
    result.amount = Money(700);
    result.value = 700;
    result.nextAction = LandingAction::BANKRUPT_CHECK;
    return result;
}

std::string DoctorFeeCard::getId() const {
    return "DoctorFeeCard";
}
