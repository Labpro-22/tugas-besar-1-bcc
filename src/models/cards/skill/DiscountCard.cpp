#include "models/cards/skill/DiscountCard.hpp"

#include <stdexcept>

DiscountCard::DiscountCard(int percentage, int duration)
    : SkillCard(
          "DiscountCard",
          "Diskon " + std::to_string(percentage) + "% selama " + std::to_string(duration) + " giliran."
      ),
      percentage(percentage),
      duration(duration) {
    if (percentage < 0 || percentage > 100) {
        throw std::invalid_argument("DiscountCard percentage must be between 0 and 100.");
    }
    if (duration <= 0) {
        throw std::invalid_argument("DiscountCard duration must be positive.");
    }
}

DiscountCard::~DiscountCard() = default;

CardResult DiscountCard::activate(Player&, GameContext&) {
    CardResult result = CardResult::withAction(
        CardResultAction::APPLY_DISCOUNT,
        "DiscountCard digunakan. Diskon " + std::to_string(percentage) + "% aktif."
    );
    result.value = percentage;
    result.percentage = percentage;
    result.duration = duration;
    return result;
}

SkillCardType DiscountCard::getCardType() const {
    return SkillCardType::DISCOUNT;
}

std::string DiscountCard::getSaveValue() const {
    return std::to_string(percentage);
}

std::string DiscountCard::getSaveDuration() const {
    return std::to_string(duration);
}

std::string DiscountCard::getId() const {
    return "DiscountCard";
}

int DiscountCard::getPercentage() const {
    return percentage;
}

int DiscountCard::getDuration() const {
    return duration;
}
