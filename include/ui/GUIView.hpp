#pragma once

#include "ui/AppScreen.hpp"
#include "ui/IGameView.hpp"
#include "utils/Types.hpp"

namespace sf {
class RenderWindow;
class Event;
}

class GUIView final : public IGameView {
public:
    explicit GUIView(sf::RenderWindow& window);

    AppScreen  screen() const { return screen_; }
    SetupState& setup()       { return setup_; }

    bool handleMenuEvent(const sf::Event& event);
    void renderCurrentScreen();

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

private:
    [[maybe_unused]] sf::RenderWindow* window;
    AppScreen  screen_{AppScreen::LANDING};
    SetupState setup_;
    [[maybe_unused]] int custPlayerTab_{0};
    [[maybe_unused]] int hoveredItem_{-1};

    vector<LogEntry> log_;
    int  lastD1_{0}, lastD2_{0};
    bool diceRolled_{false};
    [[maybe_unused]] int logScrollOffset_{0};

    void drawLeftPanel (sf::RenderWindow& rw, const GameStateView& state);
    void drawRightPanel(sf::RenderWindow& rw, const GameStateView& state);
    void drawBottomStrip(sf::RenderWindow& rw, const GameStateView& state);
};
