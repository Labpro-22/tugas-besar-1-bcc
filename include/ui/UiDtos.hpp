#pragma once

#include <string>
#include <vector>

#include "models/Money.hpp"
#include "utils/Enums.hpp"
#include "utils/Types.hpp"

// Lightweight DTOs for UI layer.
// Keep these free from heavy model includes to avoid circular dependencies.

class PropertyInfo {
public:
    string code;
    string name;
    string ownerName;  // empty if BANK
    PropertyStatus status{PropertyStatus::BANK};
    int buildingLevel{0};
    Money purchasePrice{0};
};

class RentInfo {
public:
    string payerName;
    string ownerName;
    Money amount{0};
    PropertyInfo property;
};

class TaxInfo {
public:
    int flatAmount{0};
    int percentageAmount{0};
};

class AuctionState {
public:
    int currentBid{0};
    string highestBidderName;
    vector<string> participantNames;
    PropertyInfo property;
};

class BankruptcyInfo {
public:
    string playerName;
    Money obligation{0};
    string creditorName;  // empty if BANK
};

class LiquidationState {
public:
    Money obligation{0};
    Money maxLiquidation{0};
    vector<string> options;
};

class CardInfo {
public:
    string name;
    string description;
    string type;
};

class WinnerInfo {
public:
    vector<string> winners;
};

class JailInfo {
public:
    string playerName;
    int turnsRemaining{0};
    int fine{0};
};

class BuildMenuState {
public:
    vector<string> options;
};

class MortgageMenuState {
public:
    vector<string> options;
};

class RedeemMenuState {
public:
    vector<string> options;
};
