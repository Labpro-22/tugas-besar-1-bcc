#include "ui/GUIInput.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

#if NIMONSPOLY_ENABLE_RAYLIB
#include "ui/RaylibCompat.hpp"
#endif

const GUIPromptState& GUIInput::currentPrompt() const {
    return prompt_;
}

void GUIInput::setRenderCallback(GUIRenderCallback cb) {
    renderCallback_ = std::move(cb);
}

void GUIInput::activatePrompt(GUIPromptType type, const string& label,
                              int minVal, int maxVal,
                              const vector<string>& options) {
    prompt_ = GUIPromptState{};
    prompt_.type = type;
    prompt_.label = label;
    prompt_.minVal = minVal;
    prompt_.maxVal = maxVal;
    prompt_.options = options;
}

void GUIInput::confirmPrompt() {
    const string& buf = prompt_.textBuffer;

    switch (prompt_.type) {
        case GUIPromptType::YES_NO: {
            if (!buf.empty()) {
                const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(buf[0])));
                if (c == 'y') {
                    prompt_.resBool = true;
                    prompt_.resolved = true;
                } else if (c == 'n') {
                    prompt_.resBool = false;
                    prompt_.resolved = true;
                }
            }
            break;
        }
        case GUIPromptType::COMMAND:
        case GUIPromptType::TEXT:
        case GUIPromptType::PLAYER_NAME:
        case GUIPromptType::PROPERTY_CODE: {
            if (!buf.empty()) {
                prompt_.resString = buf;
                prompt_.resolved = true;
            }
            break;
        }
        case GUIPromptType::MENU_CHOICE:
        case GUIPromptType::TAX_CHOICE:
        case GUIPromptType::LIQUIDATION:
        case GUIPromptType::SKILL_CARD:
        case GUIPromptType::PLAYER_COUNT:
        case GUIPromptType::NUMBER: {
            try {
                const int n = std::stoi(buf);
                if (n >= prompt_.minVal && n <= prompt_.maxVal) {
                    prompt_.resInt = n;
                    prompt_.resolved = true;
                }
            } catch (...) {
            }
            break;
        }
        case GUIPromptType::DICE_MANUAL: {
            std::istringstream iss(buf);
            int d1 = 0;
            int d2 = 0;
            if ((iss >> d1 >> d2) && d1 >= 1 && d1 <= 6 && d2 >= 1 && d2 <= 6) {
                prompt_.resInt = d1 * 10 + d2;
                prompt_.resolved = true;
            }
            break;
        }
        case GUIPromptType::AUCTION: {
            string upper = buf;
            std::transform(upper.begin(), upper.end(), upper.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            if (upper == "PASS") {
                prompt_.resAuction = {AuctionAction::PASS, 0};
                prompt_.resolved = true;
            } else if (upper.rfind("BID", 0) == 0) {
                try {
                    int amount = std::stoi(buf.substr(3));
                    if (amount > prompt_.minVal && amount <= prompt_.maxVal) {
                        prompt_.resAuction = {AuctionAction::BID, amount};
                        prompt_.resolved = true;
                    }
                } catch (...) {
                }
            }
            break;
        }
        default:
            break;
    }
}

void GUIInput::appendTextInput() {
#if NIMONSPOLY_ENABLE_RAYLIB
    for (int codepoint = GetCharPressed(); codepoint > 0; codepoint = GetCharPressed()) {
        if (codepoint >= 32 && codepoint < 127) {
            prompt_.textBuffer += static_cast<char>(codepoint);
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !prompt_.textBuffer.empty()) {
        prompt_.textBuffer.pop_back();
    }
#endif
}

bool GUIInput::updatePrompt() {
#if NIMONSPOLY_ENABLE_RAYLIB
    if (prompt_.type == GUIPromptType::NONE || prompt_.resolved) {
        return false;
    }

    if ((prompt_.type == GUIPromptType::DICE_MANUAL ||
         prompt_.type == GUIPromptType::MENU_CHOICE ||
         prompt_.type == GUIPromptType::LIQUIDATION ||
         prompt_.type == GUIPromptType::SKILL_CARD) &&
        IsKeyPressed(KEY_ESCAPE)) {
        prompt_.cancelled = true;
        prompt_.resolved = true;
        return true;
    }

    appendTextInput();

    if (prompt_.type == GUIPromptType::MENU_CHOICE ||
        prompt_.type == GUIPromptType::SKILL_CARD ||
        prompt_.type == GUIPromptType::LIQUIDATION ||
        prompt_.type == GUIPromptType::PLAYER_COUNT ||
        prompt_.type == GUIPromptType::NUMBER ||
        prompt_.type == GUIPromptType::TAX_CHOICE) {
        for (int digit = 0; digit <= 9; ++digit) {
            const int key = KEY_ZERO + digit;
            const int keypad = KEY_KP_0 + digit;
            if (IsKeyPressed(key) || IsKeyPressed(keypad)) {
                if (prompt_.type == GUIPromptType::MENU_CHOICE && digit == 0) {
                    prompt_.cancelled = true;
                    prompt_.resolved = true;
                    return true;
                }
                if (digit >= prompt_.minVal && digit <= prompt_.maxVal) {
                    prompt_.textBuffer = std::to_string(digit);
                    confirmPrompt();
                }
            }
        }
    }

    if (prompt_.type == GUIPromptType::YES_NO) {
        if (IsKeyPressed(KEY_Y)) {
            prompt_.resBool = true;
            prompt_.resolved = true;
        } else if (IsKeyPressed(KEY_N)) {
            prompt_.resBool = false;
            prompt_.resolved = true;
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        confirmPrompt();
    }

    return prompt_.resolved;
#else
    return false;
#endif
}

void GUIInput::waitForResolution() {
#if NIMONSPOLY_ENABLE_RAYLIB
    while (!prompt_.resolved && !WindowShouldClose()) {
        updatePrompt();
        if (renderCallback_) {
            renderCallback_();
        }
        WaitTime(1.0 / 120.0);
    }
#else
    prompt_.resolved = true;
#endif
}

int GUIInput::getPlayerCount() {
    activatePrompt(GUIPromptType::PLAYER_COUNT, "Jumlah pemain (2-4)", 2, 4);
    waitForResolution();
    return prompt_.resInt;
}

string GUIInput::getPlayerName(int playerIdx) {
    activatePrompt(GUIPromptType::PLAYER_NAME, "Nama pemain " + std::to_string(playerIdx));
    waitForResolution();
    return prompt_.resString;
}

string GUIInput::getCommand() {
    activatePrompt(GUIPromptType::COMMAND, "Masukkan perintah");
    waitForResolution();
    string cmd = prompt_.resString;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return cmd;
}

int GUIInput::getMenuChoice(const vector<string>& options) {
    activatePrompt(GUIPromptType::MENU_CHOICE, "Pilih opsi", 1, static_cast<int>(options.size()), options);
    waitForResolution();
    return prompt_.resInt;
}

bool GUIInput::getYesNo(const string& promptText) {
    activatePrompt(GUIPromptType::YES_NO, promptText);
    waitForResolution();
    return prompt_.resBool;
}

int GUIInput::getNumberInRange(const string& promptText, int min, int max) {
    activatePrompt(GUIPromptType::NUMBER, promptText, min, max);
    waitForResolution();
    return prompt_.resInt;
}

string GUIInput::getString(const string& promptText) {
    activatePrompt(GUIPromptType::TEXT, promptText);
    waitForResolution();
    return prompt_.resString;
}

pair<int, int> GUIInput::getManualDice() {
    activatePrompt(GUIPromptType::DICE_MANUAL, "Dadu manual (D1 D2, misal: 3 5)");
    waitForResolution();
    if (prompt_.cancelled) {
        return {0, 0};
    }
    return {prompt_.resInt / 10, prompt_.resInt % 10};
}

AuctionDecision GUIInput::getAuctionDecision(const string& playerName, int currentBid, int playerMoney) {
    activatePrompt(GUIPromptType::AUCTION,
                   "Lelang - giliran " + playerName + " (PASS / BID <jumlah>)",
                   currentBid,
                   playerMoney);
    waitForResolution();
    return prompt_.resAuction;
}

TaxChoice GUIInput::getTaxChoice() {
    activatePrompt(GUIPromptType::TAX_CHOICE, "Pilih opsi pajak (1=flat, 2=%)", 1, 2);
    waitForResolution();
    return (prompt_.resInt == 1) ? TaxChoice::FLAT : TaxChoice::PERCENTAGE;
}

int GUIInput::getLiquidationChoice(const LiquidationState& liquidationState) {
    vector<string> options;
    options.reserve(liquidationState.options.size());
    for (const LiquidationOption& option : liquidationState.options) {
        const string action = option.type == LiquidationType::SELL ? "JUAL" : "GADAI";
        options.push_back(action + " " + option.name + " (" + option.code + ") - " +
                          option.description + " - nilai " + option.value.toString());
    }

    activatePrompt(GUIPromptType::LIQUIDATION,
                   "Likuidasi - pilih aksi (0 = selesai)",
                   0,
                   static_cast<int>(liquidationState.options.size()),
                   options);
    waitForResolution();
    return prompt_.resInt;
}

int GUIInput::getSkillCardChoice(const vector<CardInfo>& cards) {
    vector<string> options;
    options.reserve(cards.size() * 2);
    for (size_t i = 0; i < cards.size(); ++i) {
        const CardInfo& card = cards[i];
        options.push_back(std::to_string(i + 1) + " " + card.name);
        options.push_back(card.description.empty() ? "-" : card.description);
    }

    const bool discardOverflowCard = cards.size() > 3;
    activatePrompt(GUIPromptType::SKILL_CARD,
                   discardOverflowCard
                       ? "Pilih kartu yang dibuang (0 = buang kartu baru)"
                       : "Pilih kartu (0 = batal)",
                   0,
                   static_cast<int>(cards.size()),
                   options);
    waitForResolution();
    return prompt_.resInt;
}

string GUIInput::getPropertyCodeInput(const string& promptText) {
    activatePrompt(GUIPromptType::PROPERTY_CODE, promptText);
    waitForResolution();
    string code = prompt_.resString;
    std::transform(code.begin(), code.end(), code.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return code;
}
