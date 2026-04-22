#include "models/cards/CardSystem.hpp"

#include <array>
#include <random>
#include <utility>
#include <vector>

#include "core/Bank.hpp"
#include "models/Player.hpp"
#include "models/effects/DiscountEffect.hpp"
#include "models/effects/ShieldEffect.hpp"
#include "models/cards/chance/ChanceGoToJailCard.hpp"
#include "models/cards/chance/ChanceGoToNearestStationCard.hpp"
#include "models/cards/chance/ChanceMoveBackThreeCard.hpp"
#include "models/cards/community/BirthdayCard.hpp"
#include "models/cards/community/DoctorFeeCard.hpp"
#include "models/cards/community/ElectionCampaignCard.hpp"
#include "models/cards/skill/DemolitionCard.hpp"
#include "models/cards/skill/DiscountCard.hpp"
#include "models/cards/skill/LassoCard.hpp"
#include "models/cards/skill/MoveCard.hpp"
#include "models/cards/skill/ShieldCard.hpp"
#include "models/cards/skill/TeleportCard.hpp"
#include "utils/Types.hpp"

namespace {
    int randomInt(int minimum, int maximum) {
        static std::random_device randomDevice;
        static std::mt19937 generator(randomDevice());
        std::uniform_int_distribution<int> distribution(minimum, maximum);
        return distribution(generator);
    }

    int randomDiscountPercentage() {
        static constexpr std::array<int, 5> values{10, 20, 30, 40, 50};
        return values[static_cast<std::size_t>(randomInt(0, static_cast<int>(values.size()) - 1))];
    }

    std::vector<Player*> getOtherPlayers(Player& player, const std::vector<Player*>& players) {
        std::vector<Player*> otherPlayers;
        for (Player* other : players) {
            if (other && other != &player && !other->isBankrupt()) {
                otherPlayers.push_back(other);
            }
        }
        return otherPlayers;
    }

    Money getPayableAmount(const Player& player, const Money& amount) {
        if (player.isPaymentBlocked()) {
            return Money::zero();
        }
        return player.applyOutgoingModifiers(amount);
    }

    bool canPay(Player& player, const Money& amount) {
        return player.canAfford(getPayableAmount(player, amount));
    }

    CardResult failAfterDraw(CardResult result, std::string message) {
        result.success = false;
        result.message = std::move(message);
        return result;
    }
}

CardSystem::CardSystem() {
    initializeDecks();
}

std::unique_ptr<ChanceCard> CardSystem::drawChance() {
    return chanceDeck.draw();
}

std::unique_ptr<CommunityChestCard> CardSystem::drawCommunityChest() {
    return communityChestDeck.draw();
}

std::unique_ptr<SkillCard> CardSystem::drawSkill() {
    return skillDeck.draw();
}

void CardSystem::discardChance(std::unique_ptr<ChanceCard> card) {
    chanceDeck.discard(std::move(card));
}

void CardSystem::discardCommunityChest(std::unique_ptr<CommunityChestCard> card) {
    communityChestDeck.discard(std::move(card));
}

void CardSystem::discardSkill(std::unique_ptr<SkillCard> card) {
    skillDeck.discard(std::move(card));
}

void CardSystem::initializeDecks() {
    chanceDeck.clear();
    communityChestDeck.clear();
    skillDeck.clear();

    chanceDeck.emplaceCard<ChanceGoToNearestStationCard>();
    chanceDeck.emplaceCard<ChanceMoveBackThreeCard>();
    chanceDeck.emplaceCard<ChanceGoToJailCard>();

    communityChestDeck.emplaceCard<BirthdayCard>();
    communityChestDeck.emplaceCard<DoctorFeeCard>();
    communityChestDeck.emplaceCard<ElectionCampaignCard>();

    for (int i = 0; i < 4; ++i) {
        skillDeck.emplaceCard<MoveCard>(randomInt(1, 6));
    }

    for (int i = 0; i < 3; ++i) {
        skillDeck.emplaceCard<DiscountCard>(randomDiscountPercentage(), 1);
    }

    for (int i = 0; i < 2; ++i) {
        skillDeck.emplaceCard<ShieldCard>(1);
        skillDeck.emplaceCard<TeleportCard>();
        skillDeck.emplaceCard<LassoCard>();
        skillDeck.emplaceCard<DemolitionCard>();
    }

    chanceDeck.shuffle();
    communityChestDeck.shuffle();
    skillDeck.shuffle();
}

CardResult CardSystem::applyImmediateResult(Player& player, GameContext& context, const CardResult& result) {
    if (!result.success) {
        return result;
    }

    CardResult applied = result;
    const std::vector<Player*> otherPlayers = getOtherPlayers(player, context.players);

    switch (result.action) {
        case CardResultAction::RECEIVE_FROM_EACH_PLAYER:
            for (Player* other : otherPlayers) {
                if (!canPay(*other, result.amount)) {
                    return failAfterDraw(result, other->getUsername() + " tidak memiliki cukup uang untuk membayar kartu.");
                }
            }
            for (Player* other : otherPlayers) {
                context.bank.transferBetweenPlayers(*other, player, result.amount, result.message);
            }
            break;

        case CardResultAction::PAY_BANK:
            if (!canPay(player, result.amount)) {
                return failAfterDraw(result, player.getUsername() + " tidak memiliki cukup uang untuk membayar kartu.");
            }
            context.bank.collectFromPlayer(player, result.amount, result.message);
            break;

        case CardResultAction::PAY_EACH_PLAYER: {
            const Money payableAmount = getPayableAmount(player, result.amount);
            const Money totalAmount(payableAmount.getAmount() * static_cast<int>(otherPlayers.size()));
            if (!player.canAfford(totalAmount)) {
                return failAfterDraw(result, player.getUsername() + " tidak memiliki cukup uang untuk membayar semua pemain.");
            }
            for (Player* other : otherPlayers) {
                context.bank.transferBetweenPlayers(player, *other, result.amount, result.message);
            }
            break;
        }

        case CardResultAction::APPLY_DISCOUNT:
            player.addEffect(new DiscountEffect(result.percentage, result.duration));
            break;

        case CardResultAction::APPLY_SHIELD:
            player.addEffect(new ShieldEffect(result.duration));
            break;

        case CardResultAction::SEND_TO_JAIL:
            player.setStatus(PlayerStatus::JAILED);
            player.setConsecutiveDoubles(0);
            player.resetJailTurns();
            break;

        case CardResultAction::NONE:
        case CardResultAction::MOVE_RELATIVE:
        case CardResultAction::MOVE_TO_NEAREST_STATION:
        case CardResultAction::TELEPORT:
        case CardResultAction::LASSO:
        case CardResultAction::DEMOLISH_PROPERTY:
            break;
    }

    return applied;
}

CardDeck<ChanceCard>& CardSystem::getChanceDeck() {
    return chanceDeck;
}

const CardDeck<ChanceCard>& CardSystem::getChanceDeck() const {
    return chanceDeck;
}

CardDeck<CommunityChestCard>& CardSystem::getCommunityChestDeck() {
    return communityChestDeck;
}

const CardDeck<CommunityChestCard>& CardSystem::getCommunityChestDeck() const {
    return communityChestDeck;
}

CardDeck<SkillCard>& CardSystem::getSkillDeck() {
    return skillDeck;
}

const CardDeck<SkillCard>& CardSystem::getSkillDeck() const {
    return skillDeck;
}
