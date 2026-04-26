#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ui/IGameInput.hpp"

using GUIRenderCallback = std::function<void()>;

enum class GUIPromptType {
    NONE,
    COMMAND,
    YES_NO,
    MENU_CHOICE,
    NUMBER,
    TEXT,
    DICE_MANUAL,
    AUCTION,
    TAX_CHOICE,
    LIQUIDATION,
    SKILL_CARD,
    PROPERTY_CODE,
    PLAYER_COUNT,
    PLAYER_NAME,
};

struct GUIPromptState {
    GUIPromptType type{GUIPromptType::NONE};
    string         label;
    int            minVal{0};
    int            maxVal{0};
    vector<string> options;
    string         textBuffer;

    bool           resolved{false};
    bool           cancelled{false};
    bool           resBool{false};
    int            resInt{0};
    string         resString;
    AuctionDecision resAuction{};
};

class GUIInput final : public IGameInput {
public:
    GUIInput() = default;

    bool updatePrompt();
    const GUIPromptState& currentPrompt() const;
    void setRenderCallback(GUIRenderCallback cb);

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
    int       getLiquidationChoice(const LiquidationState& liquidationState) override;
    int       getSkillCardChoice(const vector<CardInfo>& cards) override;
    string    getPropertyCodeInput(const string& prompt) override;

private:
    GUIPromptState    prompt_;
    GUIRenderCallback renderCallback_;

    void waitForResolution();
    void activatePrompt(GUIPromptType type, const string& label,
                        int minVal = 0, int maxVal = 0,
                        const vector<string>& options = {});
    void confirmPrompt();
    void appendTextInput();
};
