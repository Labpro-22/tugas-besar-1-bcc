#pragma once

#include <memory>

#include "models/cards/CardDeck.hpp"
#include "models/cards/ChanceCard.hpp"
#include "models/cards/CommunityChestCard.hpp"
#include "models/cards/SkillCard.hpp"

class GameContext;
class Player;

class CardSystem {
    public:
        CardSystem();
        ~CardSystem() = default;

        CardSystem(const CardSystem& other) = delete;
        CardSystem& operator=(const CardSystem& other) = delete;

        CardSystem(CardSystem&& other) noexcept = default;
        CardSystem& operator=(CardSystem&& other) noexcept = default;

        std::unique_ptr<ChanceCard> drawChance();
        std::unique_ptr<CommunityChestCard> drawCommunityChest();
        std::unique_ptr<SkillCard> drawSkill();

        void discardChance(std::unique_ptr<ChanceCard> card);
        void discardCommunityChest(std::unique_ptr<CommunityChestCard> card);
        void discardSkill(std::unique_ptr<SkillCard> card);

        void initializeDecks();
        static CardResult applyImmediateResult(Player& player, GameContext& context, const CardResult& result);

        CardDeck<ChanceCard>& getChanceDeck();
        const CardDeck<ChanceCard>& getChanceDeck() const;
        CardDeck<CommunityChestCard>& getCommunityChestDeck();
        const CardDeck<CommunityChestCard>& getCommunityChestDeck() const;
        CardDeck<SkillCard>& getSkillDeck();
        const CardDeck<SkillCard>& getSkillDeck() const;

    private:
        CardDeck<ChanceCard> chanceDeck;
        CardDeck<CommunityChestCard> communityChestDeck;
        CardDeck<SkillCard> skillDeck;
};
