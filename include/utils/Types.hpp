#pragma once

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

class GameContext {
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

class LandingResult {
public:
    LandingAction action;
    int amount;
    Player* creditor;
    PropertyTile* property;
};

class LogEntry {
public:
    int turn;
    string username;
    string actionType;
    string detail;
};

class AuctionResult {
public:
    Player* winner;
    int finalPrice;
    PropertyTile* property;
};

class FestivalEffect {
public:
    Player* owner;
    int multiplier;
    int turnsRemaining;
    int timesApplied;
};

class PlayerView {
public:
    string username;
    Money money;
    int position;
    PlayerStatus status;
    int propertyCount;
    int skillCardCount;
};

class PropertyView {
public:
    string code;
    string name;
    string ownerName;
    PropertyStatus status;
    int buildingLevel;
};
