#include "models/cards/skill/DemolitionCard.hpp"

DemolitionCard::DemolitionCard()
    : SkillCard("DemolitionCard", "Hancurkan satu properti milik pemain lawan.") {}

DemolitionCard::~DemolitionCard() = default;

CardResult DemolitionCard::activate(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::DEMOLISH_PROPERTY,
        "DemolitionCard digunakan. Pilih properti lawan yang akan dihancurkan."
    );
    result.requiresTargetSelection = true;
    return result;
}

SkillCardType DemolitionCard::getCardType() const {
    return SkillCardType::DEMOLITION;
}

std::string DemolitionCard::getId() const {
    return "DemolitionCard";
}
