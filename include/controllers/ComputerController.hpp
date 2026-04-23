#pragma once

#include <string>
#include <vector>

#include "controllers/PlayerController.hpp"
#include "core/state/header/GameStateView.hpp"

class ComputerController final : public PlayerController {
public:
    ComputerController();
    explicit ComputerController(const string& playerName);

    string chooseCommand(const GameStateView& state) override;

    bool decideBuyProperty(const PropertyInfo& info, Money money) override;
    AuctionDecision decideAuction(int currentBid, Money money) override;
    TaxChoice decideTax(int flatAmount, int percentAmount) override;

    int decideSkillCard(const vector<CardInfo>& cards) override;
    string decideFestivalProperty(const vector<PropertyInfo>& props) override;

    int decideBuild(const BuildMenuState& state) override;
    int decideLiquidation(const LiquidationState& state) override;
    int decideDropCard(const vector<CardInfo>& cards) override;

    bool decideJailPay() override;
    string decideTeleportTarget() override;
    string decideLassoTarget(const vector<string>& players) override;
    string decideDemolitionTarget(const vector<PropertyInfo>& properties) override;

private:
    string playerName_;
    GameStateView lastState_;
    int lastTurn_;
    int lastActivePlayerIndex_;
    string lastActivePlayerName_;
    bool attemptedSkillThisTurn_;
    bool attemptedJailFineThisTurn_;

    void refreshTurnMemory(const GameStateView& state);
    const PlayerView* activePlayerView(const GameStateView& state) const;

    bool shouldTrySkillCard(const PlayerView& player, const GameStateView& state) const;
    bool shouldPayJailFine(const PlayerView& player) const;

    int cashReserve(int cash) const;
    int propertyScore(const PropertyInfo& info) const;
    int propertyCodeScore(const string& code) const;
    int cardScore(const CardInfo& card) const;
    int liquidationReturnIndex(const LiquidationState& state, int optionPosition) const;

    string bestTeleportTarget() const;
    string fallbackSafeTileCode() const;
};
