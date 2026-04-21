#include "models/cards/skill/LassoCard.hpp"

LassoCard::LassoCard()
    : SkillCard("LassoCard", "Tarik satu pemain lawan di depan posisi pemain.") {}

LassoCard::~LassoCard() = default;

CardResult LassoCard::activate(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::LASSO,
        "LassoCard digunakan. Pilih pemain lawan yang akan ditarik."
    );
    result.requiresTargetSelection = true;
    return result;
}

SkillCardType LassoCard::getCardType() const {
    return SkillCardType::LASSO;
}

std::string LassoCard::getId() const {
    return "LassoCard";
}
