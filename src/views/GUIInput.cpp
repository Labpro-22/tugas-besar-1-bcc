#include "ui/GUIInput.hpp"

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>

#if NIMONSPOLY_ENABLE_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#endif

GUIInput::GUIInput(sf::RenderWindow& window) : window_(&window) {}


const GUIPromptState& GUIInput::currentPrompt() const { return prompt_; }

void GUIInput::setRenderCallback(GUIRenderCallback cb) {
    renderCallback_ = std::move(cb);
}


void GUIInput::activatePrompt(GUIPromptType type, const string& label,
                               int minVal, int maxVal,
                               const vector<string>& options) {
    prompt_ = GUIPromptState{};
    prompt_.type    = type;
    prompt_.label   = label;
    prompt_.minVal  = minVal;
    prompt_.maxVal  = maxVal;
    prompt_.options = options;
}

void GUIInput::confirmPrompt() {
    const string& buf = prompt_.textBuffer;

    switch (prompt_.type) {
        case GUIPromptType::YES_NO: {
            if (!buf.empty()) {
                char c = static_cast<char>(tolower(static_cast<unsigned char>(buf[0])));
                if (c == 'y') { prompt_.resBool = true;  prompt_.resolved = true; }
                if (c == 'n') { prompt_.resBool = false; prompt_.resolved = true; }
            }
            break;
        }
        case GUIPromptType::COMMAND:
        case GUIPromptType::TEXT:
        case GUIPromptType::PLAYER_NAME:
        case GUIPromptType::PROPERTY_CODE: {
            if (!buf.empty()) {
                prompt_.resString = buf;
                prompt_.resolved  = true;
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
                int n = stoi(buf);
                if (n >= prompt_.minVal && n <= prompt_.maxVal) {
                    prompt_.resInt   = n;
                    prompt_.resolved = true;
                }
            } catch (...) {}
            break;
        }
        case GUIPromptType::DICE_MANUAL: {
            // Expected format: "D1 D2"
            istringstream iss(buf);
            int d1, d2;
            if ((iss >> d1 >> d2) && d1 >= 1 && d1 <= 6 && d2 >= 1 && d2 <= 6) {
                prompt_.resAuction = {};  // not used here
                prompt_.resInt     = d1 * 10 + d2;  // pack: d1 in tens, d2 in units
                prompt_.resolved   = true;
            }
            break;
        }
        case GUIPromptType::AUCTION: {
            // Parse "PASS" or "BID <amount>"
            string upper = buf;
            transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == "PASS") {
                prompt_.resAuction = {AuctionAction::PASS, 0};
                prompt_.resolved   = true;
            } else if (upper.size() > 3 && upper.substr(0, 3) == "BID") {
                try {
                    int amount = stoi(buf.substr(3));
                    if (amount > prompt_.minVal && amount <= prompt_.maxVal) {
                        prompt_.resAuction = {AuctionAction::BID, amount};
                        prompt_.resolved   = true;
                    }
                } catch (...) {}
            }
            break;
        }
        default:
            break;
    }
}

void GUIInput::handleTextEvent([[maybe_unused]] const sf::Event& event) {
#if NIMONSPOLY_ENABLE_SFML
    if (const auto* te = event.getIf<sf::Event::TextEntered>()) {
        uint32_t cp = te->unicode;
        if (cp == '\r' || cp == '\n') {
            confirmPrompt();
        } else if (cp == 8 || cp == 127) {  // Backspace / DEL
            if (!prompt_.textBuffer.empty()) prompt_.textBuffer.pop_back();
        } else if (cp >= 32 && cp < 127) {
            prompt_.textBuffer += static_cast<char>(cp);
        }
    }

    // Also handle KeyPressed for menu shortcuts (number keys)
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        // Allow clicking numbered keys directly for MENU_CHOICE
        if (prompt_.type == GUIPromptType::MENU_CHOICE ||
            prompt_.type == GUIPromptType::SKILL_CARD  ||
            prompt_.type == GUIPromptType::LIQUIDATION) {
            int num = -1;
            if (kp->code >= Key::Num0 && kp->code <= Key::Num9)
                num = static_cast<int>(kp->code) - static_cast<int>(Key::Num0);
            else if (kp->code >= Key::Numpad0 && kp->code <= Key::Numpad9)
                num = static_cast<int>(kp->code) - static_cast<int>(Key::Numpad0);
            if (num >= prompt_.minVal && num <= prompt_.maxVal) {
                prompt_.textBuffer = to_string(num);
                confirmPrompt();
            }
        }
        // Y/N shortcuts for YES_NO
        if (prompt_.type == GUIPromptType::YES_NO) {
            if (kp->code == Key::Y) { prompt_.resBool = true;  prompt_.resolved = true; }
            if (kp->code == Key::N) { prompt_.resBool = false; prompt_.resolved = true; }
        }
        // Enter confirms
        if (kp->code == Key::Enter) {
            confirmPrompt();
        }
    }
#endif
}

bool GUIInput::handleEvent(const sf::Event& event) {
    if (prompt_.type == GUIPromptType::NONE || prompt_.resolved) return false;
    handleTextEvent(event);
    return prompt_.resolved;
}


void GUIInput::waitForResolution() {
#if NIMONSPOLY_ENABLE_SFML
    if (!window_) { prompt_.resolved = true; return; }
    while (!prompt_.resolved && window_->isOpen()) {
        while (const auto event = window_->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window_->close();
                return;
            }
            handleEvent(*event);
        }
        if (renderCallback_) renderCallback_();
        // Yield CPU to avoid spinning at 100%
        sf::sleep(sf::milliseconds(8));
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
    activatePrompt(GUIPromptType::PLAYER_NAME,
                   "Nama pemain " + to_string(playerIdx));
    waitForResolution();
    return prompt_.resString;
}

string GUIInput::getCommand() {
    activatePrompt(GUIPromptType::COMMAND, "Masukkan perintah");
    waitForResolution();
    string cmd = prompt_.resString;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    return cmd;
}

int GUIInput::getMenuChoice(const vector<string>& options) {
    activatePrompt(GUIPromptType::MENU_CHOICE, "Pilih opsi",
                   0, static_cast<int>(options.size()), options);
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
    int packed = prompt_.resInt;
    return {packed / 10, packed % 10};
}

AuctionDecision GUIInput::getAuctionDecision(
    const string& playerName, int currentBid, int playerMoney)
{
    activatePrompt(GUIPromptType::AUCTION,
                   "Lelang — giliran " + playerName + " (PASS / BID <jumlah>)",
                   currentBid,   // minVal reused as currentBid floor
                   playerMoney); // maxVal reused as playerMoney ceiling
    waitForResolution();
    return prompt_.resAuction;
}

TaxChoice GUIInput::getTaxChoice() {
    activatePrompt(GUIPromptType::TAX_CHOICE, "Pilih opsi pajak (1=flat, 2=%)", 1, 2);
    waitForResolution();
    return (prompt_.resInt == 1) ? TaxChoice::FLAT : TaxChoice::PERCENTAGE;
}

int GUIInput::getLiquidationChoice(int numOptions) {
    activatePrompt(GUIPromptType::LIQUIDATION,
                   "Likuidasi — pilih aksi (0 = selesai)",
                   0, numOptions);
    waitForResolution();
    return prompt_.resInt;
}

int GUIInput::getSkillCardChoice(int numCards) {
    activatePrompt(GUIPromptType::SKILL_CARD,
                   "Pilih kartu (0 = batal)",
                   0, numCards);
    waitForResolution();
    return prompt_.resInt;
}

string GUIInput::getPropertyCodeInput(const string& promptText) {
    activatePrompt(GUIPromptType::PROPERTY_CODE, promptText);
    waitForResolution();
    string code = prompt_.resString;
    transform(code.begin(), code.end(), code.begin(), ::toupper);
    return code;
}
