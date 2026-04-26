#pragma once

#include <array>
#include <string>
#include <vector>

#include "models/Money.hpp"
#include "utils/Enums.hpp"


// prevent circular includes
class Bank;
class FestivalManager;
class TransactionLogger;
class GameConfig;
class Dice;
class Player;
class Board;
class PropertyTile;
class TurnManager;

class GameContext{
public:
    Bank& bank;
    FestivalManager& festivalManager;
    TransactionLogger& logger;
    GameConfig& config;
    Dice& dice;
    vector<Player*>& players;
    Board& board;
    int currentTurn;
    class CardSystem* cardSystem;
    class AuctionManager* auctionManager;
    class BankruptcyManager* bankruptcyManager;
    class EventBus* eventBus;
    class GameState* gameState;
    TurnManager* turnManager;
};

class LandingResult{
public:
    LandingAction action;
    int amount;
    Player* creditor;
    PropertyTile* property;
};

class LogEntry{
public:
    int turn;
    string username;
    string actionType;
    string detail;
};

class AuctionResult{
public:
    Player* winner;
    int finalPrice;
    PropertyTile* property;
};

class FestivalEffect{
public:
    Player* owner;
    int multiplier;
    int turnsRemaining;
    int timesApplied;
};

class PlayerView{
public:
    string username;
    Money money;
    int position;
    PlayerStatus status;
    int propertyCount;
    int skillCardCount;
    int colorIndex{-1};
};

class PropertyView{
public:
    string code;
    string name;
    string ownerName;
    PropertyStatus status;
    TileType type{TileType::STREET};
    Color colorGroup{Color::DEFAULT};
    int purchasePrice{0};
    int mortgageValue{0};
    int buildingLevel;
    bool isMortgaged{false};
    bool monopolyComplete{false};
    int festivalMultiplier{1};
    int festivalTurnsRemaining{0};
};

class TileData{
public:
    int index{0};
    string code;
    string name;
    TileType type{TileType::GO};
    Color color{Color::DEFAULT};
    int price{0};
    int mortgageValue{0};
    string subtitle;
    PropertyStatus propertyStatus{PropertyStatus::BANK};
    string ownerName;
    int ownerColorIndex{-1};
    bool isOwnable{false};
    bool isMortgaged{false};
    int buildingLevel{0};
    int houseCount{0};
    bool hasHotel{false};
    int festivalMultiplier{1};
    int festivalTurnsRemaining{0};
    std::vector<int> rentLevels;
    int houseCost{0};
    int hotelCost{0};
};

using TileView = TileData;

class AuctionDecision{
public:
    AuctionAction action{AuctionAction::PASS};
    int bidAmount{0};
};

class PlayerSummary{
public:
    string username;
    Money money;
    int propertyCount;
    int cardCount;
    bool isBankrupt;
};

class LiquidationOption{
public:
    int index;
    LiquidationType type;
    string code;
    string name;
    Money value;
    string description;
};

class TileBuildOption{
public:
    string code;
    string name;
    int currentLevel;
    bool canBuild;
    int buildCost;
};

class ColorGroupBuildOption{
public:
    string colorName;
    Color color;
    vector<TileBuildOption> tiles;
};

class MortgageOption{
public:
    string code;
    string name;
    Color color;
    Money mortgageValue;
    bool requiresBuildingSale;
};

class RedeemOption{
public:
    string code;
    string name;
    Color color;
    Money redeemCost;
    bool canAfford;
};
