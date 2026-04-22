#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "ui/IGameInput.hpp"

class CLIInput final : public IGameInput {
public:
    CLIInput(std::istream& in = std::cin, std::ostream& out = std::cout);

    int    getPlayerCount() override;
    string getPlayerName(int playerIdx) override;

    string getCommand() override;
    int    getMenuChoice(const vector<string>& options) override;
    bool   getYesNo(const string& prompt) override;
    int    getNumberInRange(const string& prompt, int min, int max) override;
    string getString(const string& prompt) override;

    pair<int, int> getManualDice() override;

    AuctionDecision getAuctionDecision(
        const string& playerName, int currentBid, int playerMoney) override;

    TaxChoice getTaxChoice() override;
    int       getLiquidationChoice(int numOptions) override;
    int       getSkillCardChoice(int numCards) override;
    string    getPropertyCodeInput(const string& prompt) override;

private:
    std::istream& in_;
    std::ostream& out_;

    string readLine();
    int    readInt();
};
