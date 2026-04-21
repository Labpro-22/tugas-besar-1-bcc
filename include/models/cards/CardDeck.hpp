#pragma once

#include <algorithm>
#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "models/cards/Card.hpp"

template <typename T>
class CardDeck {
    public:
        using CardPtr = std::unique_ptr<T>;

        CardDeck() = default;
        ~CardDeck() = default;

        CardDeck(const CardDeck& other) = delete;
        CardDeck& operator=(const CardDeck& other) = delete;

        CardDeck(CardDeck&& other) noexcept = default;
        CardDeck& operator=(CardDeck&& other) noexcept = default;

        void addCard(CardPtr card) {
            if (!card) {
                throw std::invalid_argument("Cannot add null card to deck.");
            }
            drawCards.push_back(std::move(card));
        }

        template <typename U, typename... Args>
        U& emplaceCard(Args&&... args) {
            static_assert(std::is_base_of<T, U>::value, "U must inherit from T.");
            auto card = std::make_unique<U>(std::forward<Args>(args)...);
            U& reference = *card;
            addCard(std::move(card));
            return reference;
        }

        void shuffle() {
            static std::random_device randomDevice;
            static std::mt19937 generator(randomDevice());
            std::shuffle(drawCards.begin(), drawCards.end(), generator);
        }

        void reshuffleUsed() {
            for (auto& card : usedCards) {
                drawCards.push_back(std::move(card));
            }
            usedCards.clear();
            shuffle();
        }

        CardPtr draw() {
            if (drawCards.empty()) {
                reshuffleUsed();
            }

            if (drawCards.empty()) {
                return nullptr;
            }

            CardPtr card = std::move(drawCards.back());
            drawCards.pop_back();
            return card;
        }

        void discard(CardPtr card) {
            if (!card) {
                return;
            }
            usedCards.push_back(std::move(card));
        }

        void clear() {
            drawCards.clear();
            usedCards.clear();
        }

        bool isEmpty() const {
            return drawCards.empty() && usedCards.empty();
        }

        std::size_t getRemainingCount() const {
            return drawCards.size();
        }

        std::size_t getTotalCount() const {
            return drawCards.size() + usedCards.size();
        }

        const std::vector<CardPtr>& getDrawCards() const {
            return drawCards;
        }

        const std::vector<CardPtr>& getUsedCards() const {
            return usedCards;
        }

        void setDrawCards(std::vector<CardPtr> cards) {
            drawCards = std::move(cards);
        }

        void setUsedCards(std::vector<CardPtr> cards) {
            usedCards = std::move(cards);
        }

    private:
        std::vector<CardPtr> drawCards;
        std::vector<CardPtr> usedCards;
};
