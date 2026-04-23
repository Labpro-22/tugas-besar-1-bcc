#pragma once

#include "ui/IGameInput.hpp"

class ConsoleInput : public IGameInput {
public:
    int getPlayerCount() override;
    string getPlayerName(int playerIdx) override;

    int getMenuChoice(const vector<string>& options) override;
    string getCommand() override;
    bool getYesNo(const string& prompt) override;
    int getNumberInRange(const string& prompt, int min, int max) override;
    string getString(const string& prompt) override;
    pair<int, int> getManualDice() override;
    AuctionDecision getAuctionDecision(const string& playerName, int currentBid, int playerMoney) override;
    TaxChoice getTaxChoice() override;
    int getLiquidationChoice(int numOptions) override;
    int getSkillCardChoice(int numCards) override;
    string getPropertyCodeInput(const string& prompt) override;
};
