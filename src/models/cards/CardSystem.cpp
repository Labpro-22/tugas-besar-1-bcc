#include "models/cards/CardSystem.hpp"

#include <array>
#include <random>
#include <utility>
#include <vector>

#include "core/Bank.hpp"
#include "core/config/header/GameConfig.hpp"
#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
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
#include "tile/header/PropertyTile.hpp"
#include "tile/header/StreetTile.hpp"
#include "tile/header/Tile.hpp"
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

    int effectiveBoardSize(const GameContext& context) {
        const int size = context.board.getSize();
        return size > 0 ? size : Board::BOARD_SIZE;
    }

    int normalizePosition(int position, int boardSize) {
        if (boardSize <= 0) {
            return 0;
        }
        return (position % boardSize + boardSize) % boardSize;
    }

    int findFirstTileIndex(const Board& board, TileType type) {
        for (int i = 0; i < board.getSize(); ++i) {
            const Tile* tile = board.getTile(i);
            if (tile && tile->getType() == type) {
                return i;
            }
        }
        return -1;
    }

    int findNextRailroadIndex(const Board& board, int currentPosition, int boardSize) {
        int bestIndex = -1;
        int bestDistance = boardSize + 1;

        for (int i = 0; i < board.getSize(); ++i) {
            const Tile* tile = board.getTile(i);
            if (!tile || tile->getType() != TileType::RAILROAD) {
                continue;
            }

            int distance = normalizePosition(i - currentPosition, boardSize);
            if (distance == 0) {
                distance = boardSize;
            }

            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    void payGoSalary(Player& player, GameContext& context) {
        const int salary = context.config.getGoSalary();
        if (salary > 0) {
            context.bank.payPlayer(player, Money(salary), "Petak Mulai");
        }
    }

    void awardGoSalaryIfNeeded(Player& player, GameContext& context, int startPosition,
                               int destinationIndex, int movementOffset, bool includePassingGo) {
        const int boardSize = effectiveBoardSize(context);
        const int goIndex = findFirstTileIndex(context.board, TileType::GO);
        const int normalizedGoIndex = goIndex >= 0 ? goIndex : 0;

        if (destinationIndex == normalizedGoIndex && startPosition != destinationIndex) {
            payGoSalary(player, context);
            return;
        }

        if (includePassingGo && movementOffset > 0 && startPosition + movementOffset >= boardSize) {
            payGoSalary(player, context);
        }
    }

    CardResult movePlayerBy(Player& player, GameContext& context, CardResult applied, int movementOffset) {
        const int boardSize = effectiveBoardSize(context);
        if (boardSize <= 0) {
            return CardResult::fail("Ukuran papan tidak valid.");
        }

        const int startPosition = normalizePosition(player.getPosition(), boardSize);
        const int destinationIndex = normalizePosition(startPosition + movementOffset, boardSize);
        player.setPosition(destinationIndex);
        applied.destinationIndex = destinationIndex;
        applied.movedPlayer = true;
        awardGoSalaryIfNeeded(player, context, startPosition, destinationIndex, movementOffset, true);
        return applied;
    }

    CardResult movePlayerTo(Player& player, GameContext& context, CardResult applied,
                            int destinationIndex, bool includePassingGo) {
        const int boardSize = effectiveBoardSize(context);
        if (boardSize <= 0) {
            return CardResult::fail("Ukuran papan tidak valid.");
        }
        if (destinationIndex < 0 || destinationIndex >= boardSize) {
            return CardResult::fail("Petak tujuan kartu tidak valid.");
        }

        const int startPosition = normalizePosition(player.getPosition(), boardSize);
        const int movementOffset = includePassingGo ? normalizePosition(destinationIndex - startPosition, boardSize) : 0;
        player.setPosition(destinationIndex);
        applied.destinationIndex = destinationIndex;
        applied.movedPlayer = true;
        awardGoSalaryIfNeeded(player, context, startPosition, destinationIndex, movementOffset, includePassingGo);
        return applied;
    }

    CardResult sendPlayerToJail(Player& player, GameContext& context, CardResult applied) {
        const int jailIndex = findFirstTileIndex(context.board, TileType::JAIL);
        if (jailIndex >= 0) {
            player.setPosition(jailIndex);
            applied.destinationIndex = jailIndex;
            applied.movedPlayer = true;
        }
        player.setStatus(PlayerStatus::JAILED);
        player.setConsecutiveDoubles(0);
        player.resetJailTurns();
        return applied;
    }

    bool isAheadOfPlayer(const Player& player, const Player& target, int boardSize) {
        if (boardSize <= 0) {
            return false;
        }
        const int distance = normalizePosition(target.getPosition() - player.getPosition(), boardSize);
        return distance > 0;
    }

    CardResult pullTargetPlayer(Player& player, GameContext& context, CardResult applied, Player* targetPlayer) {
        if (!targetPlayer) {
            return CardResult::fail("Target LassoCard belum dipilih.");
        }
        if (targetPlayer == &player || targetPlayer->isBankrupt()) {
            return CardResult::fail("Target LassoCard tidak valid.");
        }

        const int boardSize = effectiveBoardSize(context);
        if (!isAheadOfPlayer(player, *targetPlayer, boardSize)) {
            return CardResult::fail("Target LassoCard harus berada di depan pemain.");
        }

        targetPlayer->setPosition(player.getPosition());
        applied.targetPlayer = targetPlayer;
        applied.movedPlayer = true;
        applied.resolveLanding = false;
        return applied;
    }

    CardResult demolishTargetProperty(Player& player, GameContext& context, CardResult applied,
                                      PropertyTile* targetProperty) {
        if (!targetProperty) {
            return CardResult::fail("Target DemolitionCard belum dipilih.");
        }

        Player* owner = targetProperty->getOwner();
        if (!owner || owner == &player || owner->isBankrupt()) {
            return CardResult::fail("Target DemolitionCard harus berupa properti milik lawan.");
        }

        if (StreetTile* street = dynamic_cast<StreetTile*>(targetProperty)) {
            while (street->getBuildingLevel() > 0) {
                street->demolish();
            }
        }

        owner->removeProperty(targetProperty);
        targetProperty->setOwner(nullptr);
        targetProperty->setStatus(PropertyStatus::BANK);
        context.board.updateMonopolies();

        applied.targetPlayer = owner;
        applied.targetProperty = targetProperty;
        return applied;
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
    chanceDeck.returnToDrawAndShuffle(std::move(card));
}

void CardSystem::discardCommunityChest(std::unique_ptr<CommunityChestCard> card) {
    communityChestDeck.returnToDrawAndShuffle(std::move(card));
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
            applied = sendPlayerToJail(player, context, applied);
            break;

        case CardResultAction::MOVE_RELATIVE:
            applied = movePlayerBy(
                player,
                context,
                applied,
                result.movementOffset != 0 ? result.movementOffset : result.value
            );
            break;

        case CardResultAction::MOVE_TO_NEAREST_STATION: {
            const int boardSize = effectiveBoardSize(context);
            const int stationIndex = findNextRailroadIndex(
                context.board,
                normalizePosition(player.getPosition(), boardSize),
                boardSize
            );
            if (stationIndex < 0) {
                return CardResult::fail("Tidak ada stasiun pada papan.");
            }
            applied = movePlayerTo(player, context, applied, stationIndex, true);
            break;
        }

        case CardResultAction::TELEPORT:
            applied = movePlayerTo(player, context, applied, result.destinationIndex, false);
            break;

        case CardResultAction::LASSO:
            applied = pullTargetPlayer(player, context, applied, result.targetPlayer);
            break;

        case CardResultAction::DEMOLISH_PROPERTY:
            applied = demolishTargetProperty(player, context, applied, result.targetProperty);
            break;

        case CardResultAction::NONE:
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
