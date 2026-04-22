#pragma once

#include <string>
#include <vector>

#include "ui/UiDtos.hpp"

class GameStateView;

class IGameView {
public:
    virtual ~IGameView() = default;

    virtual void showMainMenu() = 0;
    virtual void showPlayerOrder(const vector<string>& orderedNames) = 0;

    virtual void showBoard(const GameStateView& state) = 0;
    virtual void showTurnInfo(const string& playerName, int turnNum, int maxTurn) = 0;

    virtual void showDiceResult(int d1, int d2, const string& playerName) = 0;
    virtual void showPlayerLanding(const string& playerName, const string& tileName) = 0;
    virtual void showDoubleBonusTurn(const string& playerName, int doubleCount) = 0;

    virtual void showPropertyCard(const PropertyInfo& info) = 0;
    virtual void showPlayerProperties(const vector<PropertyInfo>& list) = 0;

    virtual void showBuyPrompt(const PropertyInfo& info, Money playerMoney) = 0;
    virtual void showRentPayment(const RentInfo& rentInfo) = 0;
    virtual void showTaxPrompt(const TaxInfo& taxInfo) = 0;

    virtual void showAuctionState(const AuctionState& state) = 0;
    virtual void showAuctionWinner(const AuctionSummary& summary) = 0;

    virtual void showFestivalPrompt(const vector<PropertyInfo>& ownedProperties) = 0;
    virtual void showFestivalReinforced(const FestivalEffectInfo& info) = 0;
    virtual void showFestivalAtMax(const FestivalEffectInfo& info) = 0;

    virtual void showJailEntry(const JailEntryInfo& info) = 0;
    virtual void showJailStatus(const JailInfo& info) = 0;

    virtual void showBankruptcy(const BankruptcyInfo& info) = 0;
    virtual void showLiquidationPanel(const LiquidationState& state) = 0;
    virtual void showLiquidationResult(bool canCover, Money finalBalance) = 0;

    virtual void showBuildMenu(const BuildMenuState& state) = 0;
    virtual void showMortgageMenu(const MortgageMenuState& state) = 0;
    virtual void showRedeemMenu(const RedeemMenuState& state) = 0;

    virtual void showCardDrawn(const CardInfo& cardInfo) = 0;
    virtual void showSkillCardHand(const vector<CardInfo>& cards) = 0;
    virtual void showCardEffect(const CardEffectInfo& info) = 0;
    virtual void showDropCardPrompt(const vector<CardInfo>& cards) = 0;

    virtual void showTransactionLog(const vector<LogEntry>& entries) = 0;
    virtual void showWinner(const WinnerInfo& winInfo) = 0;
    virtual void showSaveLoadStatus(const string& message) = 0;
    virtual void showMessage(const string& message) = 0;
};
