#include "models/cards/community/BirthdayCard.hpp"

BirthdayCard::BirthdayCard()
    : CommunityChestCard("Hari ulang tahun", "Dapatkan M100 dari setiap pemain.") {}

BirthdayCard::~BirthdayCard() = default;

CardResult BirthdayCard::execute(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::RECEIVE_FROM_EACH_PLAYER,
        "Ini adalah hari ulang tahun Anda. Dapatkan M100 dari setiap pemain."
    );
    result.amount = Money(100);
    result.value = 100;
    return result;
}

std::string BirthdayCard::getId() const {
    return "BirthdayCard";
}
