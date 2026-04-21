#include "models/cards/skill/TeleportCard.hpp"

TeleportCard::TeleportCard()
    : SkillCard("TeleportCard", "Pindah ke petak mana pun di papan permainan.") {}

TeleportCard::~TeleportCard() = default;

CardResult TeleportCard::activate(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::TELEPORT,
        "TeleportCard digunakan. Pilih petak tujuan."
    );
    result.movedPlayer = true;
    result.resolveLanding = true;
    result.requiresTargetSelection = true;
    return result;
}

SkillCardType TeleportCard::getCardType() const {
    return SkillCardType::TELEPORT;
}

std::string TeleportCard::getId() const {
    return "TeleportCard";
}
