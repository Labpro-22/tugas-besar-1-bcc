#pragma once

#include <array>
#include <string>
#include <vector>

#include "models/Money.hpp"
#include "utils/Enums.hpp"
#include "utils/Structs.hpp"


class PropertyInfo{
public:
    string code;
    string name;
    string ownerName;
    PropertyStatus status{PropertyStatus::BANK};
    TileType type{TileType::STREET};
    Color colorGroup{Color::DEFAULT};
    int buildingLevel{0};
    Money purchasePrice{0};
    Money mortgageValue{0};
    int buildCostPerHouse{0};
    int buildCostHotel{0};
    array<int, 6> rentTable{};
    int festivalMultiplier{1};
    int festivalDurationLeft{0};
};


class RentInfo{
public:
    string payerName;
    string ownerName;
    Money amount{0};
    PropertyInfo property;
    int buildingLevel{0};
    bool festivalActive{false};
    Money payerBefore{0};
    Money payerAfter{0};
    Money ownerBefore{0};
    Money ownerAfter{0};
};


class TaxInfo{
public:
    TileType taxType{TileType::TAX_PPH};
    int flatAmount{0};
    int percentageRate{0};
    int cashWealth{0};
    int propertyWealth{0};
    int buildingWealth{0};
    int totalWealth{0};
    int percentageAmount{0};
};


class AuctionState{
public:
    int currentBid{0};
    string highestBidderName;
    string currentBidderName;
    vector<string> participantNames;
    PropertyInfo property;
};

class AuctionSummary{
public:
    PropertyInfo property;
    string winnerName;
    int finalBid{0};
    bool noWinner{false};
};


class FestivalEffectInfo{
public:
    string propertyCode;
    string propertyName;
    int previousMultiplier{1};
    int newMultiplier{1};
    int durationTurns{3};
};


class BankruptcyInfo{
public:
    string playerName;
    Money obligation{0};
    string creditorName;
    bool creditorIsBank{false};
    vector<PropertyInfo> transferredAssets;
    Money transferredCash{0};
};

class LiquidationState{
public:
    Money obligation{0};
    Money maxLiquidation{0};
    Money currentBalance{0};
    vector<LiquidationOption> options;
};


class CardInfo{
public:
    string name;
    string description;
    string type;
    int value{0};
    int durationLeft{0};
};

class CardEffectInfo{
public:
    string cardType;
    string playerName;
    string detail;
};


class JailEntryInfo{
public:
    string playerName;
    JailEntryReason reason;
};

class JailInfo{
public:
    string playerName;
    int turnsInJail{0};
    int escapeAttemptsLeft{3};
    bool hasEscapeCard{false};
    int fine{0};
};


class BuildMenuState{
public:
    vector<ColorGroupBuildOption> groups;
    Money playerMoney;
};

class MortgageMenuState{
public:
    vector<MortgageOption> options;
    Money playerMoney;
};

class RedeemMenuState{
public:
    vector<RedeemOption> options;
    Money playerMoney;
};


class WinnerInfo{
public:
    vector<string> winners;
    vector<PlayerSummary> players;
    string winCondition;
};
