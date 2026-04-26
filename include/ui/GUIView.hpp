#pragma once

#include <chrono>

#include "ui/AppScreen.hpp"
#include "ui/GUIInput.hpp"
#include "ui/IGameView.hpp"
#if NIMONSPOLY_ENABLE_RAYLIB
#include "ui/RaylibCompat.hpp"
#endif
#include "utils/Types.hpp"

class GUIView final : public IGameView {
public:
    GUIView() = default;

    AppScreen  screen() const { return screen_; }
    void       setScreen(AppScreen s) { screen_ = s; }
    SetupState& setup()       { return setup_; }

    bool handleMenuInput();
    void renderCurrentScreen();

    void handleInGameClick(float mx, float my, std::string& outCommand, const GameStateView& state);

    void drawLandingPage();
    void drawNumPlayers();
    void drawCustPlayer();
    void drawCustMap();
    void drawLoadGame();

    void showMainMenu() override;
    void showPlayerOrder(const vector<string>& orderedNames) override;

    void showBoard(const GameStateView& state) override;
    void showTurnInfo(const string& playerName, int turnNum, int maxTurn) override;

    void showDiceResult(int d1, int d2, const string& playerName) override;
    void showPlayerLanding(const string& playerName, const string& tileName) override;
    void showDoubleBonusTurn(const string& playerName, int doubleCount) override;

    void showPropertyCard(const PropertyInfo& info) override;
    void showPlayerProperties(const vector<PropertyInfo>& list) override;

    void showBuyPrompt(const PropertyInfo& info, Money playerMoney) override;
    void showRentPayment(const RentInfo& rentInfo) override;
    void showTaxPrompt(const TaxInfo& taxInfo) override;

    void showAuctionState(const AuctionState& state) override;
    void showAuctionWinner(const AuctionSummary& summary) override;

    void showFestivalPrompt(const vector<PropertyInfo>& ownedProperties) override;
    void showFestivalReinforced(const FestivalEffectInfo& info) override;
    void showFestivalAtMax(const FestivalEffectInfo& info) override;

    void showJailEntry(const JailEntryInfo& info) override;
    void showJailStatus(const JailInfo& info) override;

    void showBankruptcy(const BankruptcyInfo& info) override;
    void showLiquidationPanel(const LiquidationState& state) override;
    void showLiquidationResult(bool canCover, Money finalBalance) override;

    void showBuildMenu(const BuildMenuState& state) override;
    void showMortgageMenu(const MortgageMenuState& state) override;
    void showRedeemMenu(const RedeemMenuState& state) override;

    void showCardDrawn(const CardInfo& cardInfo) override;
    void showSkillCardHand(const vector<CardInfo>& cards) override;
    void showCardEffect(const CardEffectInfo& info) override;
    void showDropCardPrompt(const vector<CardInfo>& cards) override;

    void showTransactionLog(const vector<LogEntry>& entries) override;
    void showWinner(const WinnerInfo& winInfo) override;
    void showSaveLoadStatus(const string& message) override;
    void showMessage(const string& message) override;

    void renderPromptOverlay(const struct GUIPromptState& prompt);
    void setCurrentPrompt(const GUIPromptState* p) { currentPrompt_ = p; }
    bool isDiceAnimating() const { return diceAnimating_; }
    void startGameTimer();
    std::string formattedElapsedTime() const;
    void setQueuedManualDice(int d1, int d2) {
        queuedManualDice_ = d1 > 0 && d2 > 0;
        queuedDice1_ = d1;
        queuedDice2_ = d2;
    }
    void clearQueuedManualDice() { queuedManualDice_ = false; }

private:
    AppScreen  screen_{AppScreen::LANDING};
    SetupState setup_;
    int custPlayerTab_{0};
    int hoveredItem_{-1};

    vector<LogEntry> log_;
    int  lastD1_{0}, lastD2_{0};
    int logScrollOffset_{0};
    const GUIPromptState* currentPrompt_{nullptr};
    WinnerInfo winnerInfo_;
    int promptOptionScrollPx_{0};
    GUIPromptType promptOptionScrollType_{GUIPromptType::NONE};
    int promptOptionScrollCount_{0};
    std::string promptOptionScrollLabel_;
#if NIMONSPOLY_ENABLE_RAYLIB
    std::vector<std::vector<std::string>> promptOptionWrapped_;
    std::vector<RaylibColor> promptOptionColors_;
    std::vector<float> promptOptionHeights_;
    float promptOptionTotalHeight_{0.f};
#endif

    bool diceAnimating_{false};
    float diceAnimElapsed_{0.f};
    int diceAnimFace1_{1};
    int diceAnimFace2_{1};
    bool queuedManualDice_{false};
    int queuedDice1_{1};
    int queuedDice2_{1};
    static constexpr float DICE_ANIM_DURATION = 1.2f;

    PropertyInfo buyPromptInfo_;
    Money buyPromptMoney_{0};
    bool buyPromptActive_{false};

    std::string saveLoadStatus_;
    int saveLoadStatusFrames_{0};
    std::string propertyPanelOwner_;
    std::string lastCurrentPlayerName_;
    std::chrono::steady_clock::time_point gameStartedAt_{};
    bool gameTimerRunning_{false};
    int inspectedTileIndex_{-1};
    int tileOverlayScrollPx_{0};
    int propertyPanelScrollPx_{0};

    bool handleLandingInput();
    bool handlePlayerCountInput();
    bool handlePlayerCustomizeInput();
    bool handleMapCustomizeInput();
    bool handleLoadGameInput();

#if NIMONSPOLY_ENABLE_RAYLIB
    void drawLeftPanel (const GameStateView& state, Rectangle summaryRect, Rectangle inputRect, Rectangle playersRect);
    void drawRightPanel(const GameStateView& state, Rectangle logRect, Rectangle actionsRect);
    void drawPropertyPanel(const GameStateView& state, Rectangle rect);
#endif
    void drawGameOver();
    void drawDiceAnimation(float dt);
    void drawDieFace(float cx, float cy, float size, int face,
                     unsigned fillRGB, unsigned dotRGB);
};
