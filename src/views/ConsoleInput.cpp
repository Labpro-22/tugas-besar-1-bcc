#include "ui/ConsoleInput.hpp"

#include <iostream>
#include <limits>

namespace {
    void clearLine() {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int ConsoleInput::getPlayerCount() {
    return getNumberInRange("Jumlah pemain", 2, 4);
}

string ConsoleInput::getPlayerName(int playerIdx) {
    return getString("Nama pemain " + std::to_string(playerIdx));
}

int ConsoleInput::getMenuChoice(const vector<string>& options) {
    for (std::size_t i = 0; i < options.size(); ++i) {
        std::cout << i + 1 << ". " << options[i] << "\n";
    }
    return getNumberInRange("Pilihan", 0, static_cast<int>(options.size()));
}

string ConsoleInput::getCommand() {
    std::cout << "> ";
    string command;
    std::getline(std::cin, command);
    if (command.empty()) {
        std::getline(std::cin, command);
    }
    return command;
}

bool ConsoleInput::getYesNo(const string& prompt) {
    std::cout << prompt << " ";
    string answer;
    std::getline(std::cin, answer);
    if (answer.empty()) {
        std::getline(std::cin, answer);
    }
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

int ConsoleInput::getNumberInRange(const string& prompt, int min, int max) {
    int value = min;
    while (true) {
        std::cout << prompt << " (" << min << "-" << max << "): ";
        if (std::cin >> value && value >= min && value <= max) {
            clearLine();
            return value;
        }
        std::cin.clear();
        clearLine();
        std::cout << "Input tidak valid.\n";
    }
}

string ConsoleInput::getString(const string& prompt) {
    std::cout << prompt << ": ";
    string value;
    std::getline(std::cin, value);
    if (value.empty()) {
        std::getline(std::cin, value);
    }
    return value;
}

pair<int, int> ConsoleInput::getManualDice() {
    const int d1 = getNumberInRange("Dadu 1", 1, 6);
    const int d2 = getNumberInRange("Dadu 2", 1, 6);
    return {d1, d2};
}

AuctionDecision ConsoleInput::getAuctionDecision(const string& playerName, int currentBid, int playerMoney) {
    std::cout << playerName << " uang " << playerMoney << ", bid tertinggi " << currentBid << ". BID/PASS? ";
    string action;
    std::getline(std::cin, action);
    if (action.empty()) {
        std::getline(std::cin, action);
    }
    bool isBid = !action.empty() && (action[0] == 'b' || action[0] == 'B');
    if (isBid) {
        int bid = currentBid + 1;
        std::cout << "Masukkan jumlah bid: ";
        if (std::cin >> bid) {
            clearLine();
            return {AuctionAction::BID, bid};
        }
        std::cin.clear();
        clearLine();
    }
    return {AuctionAction::PASS, 0};
}

TaxChoice ConsoleInput::getTaxChoice() {
    const int choice = getNumberInRange("Pilih pajak: 1 flat, 2 persen", 1, 2);
    return choice == 1 ? TaxChoice::FLAT : TaxChoice::PERCENTAGE;
}

int ConsoleInput::getLiquidationChoice(int numOptions) {
    return getNumberInRange("Pilih likuidasi", 0, numOptions);
}

int ConsoleInput::getSkillCardChoice(int numCards) {
    return getNumberInRange("Pilih kartu", 0, numCards);
}

string ConsoleInput::getPropertyCodeInput(const string& prompt) {
    return getString(prompt);
}
