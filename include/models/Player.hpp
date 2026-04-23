#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <memory>
#include <vector>
#include "models/Money.hpp"
#include "utils/Enums.hpp"

class PropertyTile;
class StreetTile;
class SkillCard;
class PlayerController;
class Effect;

class Player {
public:
    static constexpr int MAX_SKILL_CARDS = 3;

    Player(const std::string& username, Money startingMoney, PlayerController* controller);
    ~Player();

    Player(const Player& other) = delete;
    Player& operator=(const Player& other) = delete;
    Player(Player&& other) noexcept;
    Player& operator=(Player&& other) noexcept;

    const std::string& getUsername() const;

    Money getMoney() const;
    void addMoney(const Money& amount);
    void deductMoney(const Money& amount);
    bool canAfford(const Money& amount) const;

    int getPosition() const;
    void setPosition(int pos);
    void moveBy(int steps, int boardSize);

    PlayerStatus getStatus() const;
    void setStatus(PlayerStatus status);
    bool isActive() const;
    bool isBankrupt() const;
    bool isJailed() const;

    const std::vector<PropertyTile*>& getProperties() const;
    void addProperty(PropertyTile* tile);
    void removeProperty(PropertyTile* tile);
    std::vector<StreetTile*> getPropertiesByColor(Color color) const;
    bool hasMonopoly(Color color) const;
    int countRailroads() const;
    int countUtilities() const;

    const std::vector<SkillCard*>& getSkillCards() const;
    void addSkillCard(SkillCard* card);
    SkillCard* removeSkillCard(int index);

    const std::vector<Effect*>& getActiveEffects() const;
    void addEffect(Effect* effect);
    void tickEffects();
    Money applyOutgoingModifiers(const Money& amount) const;
    Money applyIncomingModifiers(const Money& amount, PropertyTile* prop) const;
    bool isPaymentBlocked() const;

    PlayerController* getController() const;

    Money getTotalWealth() const;

    bool getHasUsedSkillCardThisTurn() const;
    bool getHasRolledDiceThisTurn() const;
    void setHasUsedSkillCardThisTurn(bool v);
    void setHasRolledDiceThisTurn(bool v);
    void resetTurnFlags();
    void clearSkillCardsAndEffects();

    int getJailTurnsRemaining() const;
    void incrementJailTurn();
    void resetJailTurns();

    int getConsecutiveDoubles() const;
    void setConsecutiveDoubles(int n);

    int getTurnCount() const;
    void incrementTurnCount();

    bool operator<(const Player& other) const;
    bool operator>(const Player& other) const;

private:
    std::string username;
    Money money;
    int position;
    PlayerStatus status;
    std::vector<PropertyTile*> properties;
    std::vector<std::unique_ptr<SkillCard>> skillCards;
    mutable std::vector<SkillCard*> skillCardView;
    std::vector<std::unique_ptr<Effect>> activeEffects;
    mutable std::vector<Effect*> activeEffectView;
    PlayerController* controller;
    int jailTurnsRemaining;
    int consecutiveDoubles;
    bool hasUsedSkillCardThisTurn;
    bool hasRolledDiceThisTurn;
    int turnCount;
};

#endif
