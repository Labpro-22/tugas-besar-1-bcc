#include "models/cards/CardSystem.hpp"

#include <array>
#include <random>
#include <utility>

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
