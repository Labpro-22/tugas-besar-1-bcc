#pragma once

#include <array>
#include <string>
#include <vector>

#include "models/Money.hpp"
#include "utils/Enums.hpp"


class Bank;
class FestivalManager;
class TransactionLogger;
class GameConfig;
class Dice;
class Player;
class Board;
class PropertyTile;

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
};

class PropertyView{
public:
    string code;
    string name;
    string ownerName;
    PropertyStatus status;
    int buildingLevel;
};

class TileView{
public:
    int index;
    string code;
    string name;
    TileType type;
    Color color;
};

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
