#include "ui/CLIInput.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

CLIInput::CLIInput(std::istream& in, std::ostream& out) : in_(in), out_(out) {}


string CLIInput::readLine() {
    string line;
    while (getline(in_, line)) {
        // Trim trailing whitespace
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != string::npos) return line.substr(0, end + 1);
        // Empty line — retry
    }
    return "";
}

int CLIInput::readInt() {
    string line = readLine();
    try {
        return stoi(line);
    } catch (...) {
        return -1;
    }
}



int CLIInput::getPlayerCount() {
    while (true) {
        out_ << "Masukkan jumlah pemain (2-4): ";
        int n = readInt();
        if (n >= 2 && n <= 4) return n;
        out_ << "Masukan tidak valid. Masukkan angka antara 2 dan 4.\n";
    }
}

string CLIInput::getPlayerName(int playerIdx) {
    while (true) {
        out_ << "Masukkan nama pemain " << playerIdx << ": ";
        string name = readLine();
        if (!name.empty()) return name;
        out_ << "Nama tidak boleh kosong.\n";
    }
}


string CLIInput::getCommand() {
    out_ << "\n> ";
    string cmd = readLine();
    // Uppercase untuk matching command
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    return cmd;
}

int CLIInput::getMenuChoice(const vector<string>& options) {
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        out_ << (i + 1) << ". " << options[i] << "\n";
    }
    while (true) {
        out_ << "Pilih (1-" << options.size() << ", 0 untuk batal): ";
        int choice = readInt();
        if (choice >= 0 && choice <= static_cast<int>(options.size())) return choice;
        out_ << "Pilihan tidak valid.\n";
    }
}

bool CLIInput::getYesNo(const string& prompt) {
    while (true) {
        out_ << prompt << " (y/n): ";
        string s = readLine();
        if (!s.empty()) {
            char c = static_cast<char>(tolower(static_cast<unsigned char>(s[0])));
            if (c == 'y') return true;
            if (c == 'n') return false;
        }
        out_ << "Masukkan 'y' atau 'n'.\n";
    }
}

int CLIInput::getNumberInRange(const string& prompt, int min, int max) {
    while (true) {
        out_ << prompt << " (" << min << "-" << max << "): ";
        int n = readInt();
        if (n >= min && n <= max) return n;
        out_ << "Masukan tidak valid.\n";
    }
}

string CLIInput::getString(const string& prompt) {
    out_ << prompt << ": ";
    return readLine();
}


pair<int, int> CLIInput::getManualDice() {
    while (true) {
        out_ << "Masukkan nilai dadu (X Y, 1-6 masing-masing): ";
        string line = readLine();
        istringstream iss(line);
        int d1, d2;
        if ((iss >> d1 >> d2) && d1 >= 1 && d1 <= 6 && d2 >= 1 && d2 <= 6) {
            return {d1, d2};
        }
        out_ << "Masukan tidak valid. Contoh: 3 5\n";
    }
}


AuctionDecision CLIInput::getAuctionDecision(
    const string& playerName, int currentBid, int playerMoney)
{
    (void)playerName;
    while (true) {
        out_ << "Aksi (PASS / BID <jumlah>): ";
        string line = readLine();

        // Transform to uppercase for matching
        string upper = line;
        transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (upper == "PASS") {
            return {AuctionAction::PASS, 0};
        }

        if (upper.substr(0, 3) == "BID") {
            try {
                int amount = stoi(line.substr(3));
                if (amount > currentBid && amount <= playerMoney) {
                    return {AuctionAction::BID, amount};
                }
                if (amount <= currentBid) {
                    out_ << "Bid harus lebih dari penawaran tertinggi saat ini (" << currentBid << ").\n";
                } else {
                    out_ << "Bid melebihi uang yang kamu miliki (" << playerMoney << ").\n";
                }
            } catch (...) {
                out_ << "Format bid tidak valid. Contoh: BID 150\n";
            }
            continue;
        }

        out_ << "Masukkan PASS atau BID <jumlah>.\n";
    }
}


TaxChoice CLIInput::getTaxChoice() {
    while (true) {
        int choice = readInt();
        if (choice == 1) return TaxChoice::FLAT;
        if (choice == 2) return TaxChoice::PERCENTAGE;
        out_ << "Pilihan tidak valid. Masukkan 1 atau 2: ";
    }
}

int CLIInput::getLiquidationChoice(int numOptions) {
    while (true) {
        int choice = readInt();
        if (choice >= 0 && choice <= numOptions) return choice;
        out_ << "Pilihan tidak valid (0-" << numOptions << "): ";
    }
}

int CLIInput::getSkillCardChoice(int numCards) {
    while (true) {
        int choice = readInt();
        if (choice >= 0 && choice <= numCards) return choice;
        out_ << "Pilihan tidak valid (0-" << numCards << "): ";
    }
}

string CLIInput::getPropertyCodeInput(const string& prompt) {
    out_ << prompt << ": ";
    string code = readLine();
    // Uppercase for matching
    transform(code.begin(), code.end(), code.begin(), ::toupper);
    return code;
}
