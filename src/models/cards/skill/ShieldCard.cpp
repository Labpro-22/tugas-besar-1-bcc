#include "models/cards/skill/ShieldCard.hpp"

#include <stdexcept>

ShieldCard::ShieldCard(int duration)
    : SkillCard(
          "ShieldCard",
          "Kebal tagihan atau sanksi selama " + std::to_string(duration) + " giliran."
      ),
      duration(duration) {
    if (duration <= 0) {
        throw std::invalid_argument("ShieldCard duration must be positive.");
    }
}

ShieldCard::~ShieldCard() = default;

CardResult ShieldCard::activate(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::APPLY_SHIELD,
        "ShieldCard digunakan. Pemain kebal terhadap tagihan atau sanksi."
    );
    result.duration = duration;
    result.value = duration;
    return result;
}

SkillCardType ShieldCard::getCardType() const {
    return SkillCardType::SHIELD;
}

std::string ShieldCard::getId() const {
    return "ShieldCard";
}

int ShieldCard::getDuration() const {
    return duration;
}
