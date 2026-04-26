#pragma once

#include "controllers/PlayerController.hpp"
#include "ui/IGameInput.hpp"

class IGameView;

class HumanController final : public PlayerController {
public:
    explicit HumanController(IGameInput* input, IGameView* view = nullptr, const string& playerName = "");

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
    bool confirmAction(const std::string& prompt) override;
    string decideTeleportTarget() override;
    string decideLassoTarget(const vector<string>& players) override;
    string decideDemolitionTarget(const vector<PropertyInfo>& properties) override;

private:
    IGameInput* input;
    IGameView* view_;
    string playerName_;
};
