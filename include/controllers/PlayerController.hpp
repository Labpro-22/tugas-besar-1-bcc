#pragma once

#include <string>
#include <vector>

#include "ui/UiDtos.hpp"
#include "utils/Enums.hpp"

class GameStateView;

class PlayerController {
public:
    virtual ~PlayerController() = default;

    virtual string chooseCommand(const GameStateView& state) = 0;

    virtual bool decideBuyProperty(const PropertyInfo& info, Money money) = 0;
    virtual AuctionDecision decideAuction(int currentBid, Money money) = 0;
    virtual TaxChoice decideTax(int flatAmount, int percentAmount) = 0;

    virtual int decideSkillCard(const vector<CardInfo>& cards) = 0;
    virtual string decideFestivalProperty(const vector<PropertyInfo>& props) = 0;

    virtual int decideBuild(const BuildMenuState& state) = 0;
    virtual int decideLiquidation(const LiquidationState& state) = 0;
    virtual int decideDropCard(const vector<CardInfo>& cards) = 0;

    virtual bool decideJailPay() = 0;

    virtual string decideTeleportTarget() = 0;
    virtual string decideLassoTarget(const vector<string>& players) = 0;
    virtual string decideDemolitionTarget(const vector<PropertyInfo>& properties) = 0;
};
