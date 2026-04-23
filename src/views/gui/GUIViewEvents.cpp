#include "ui/GUIView.hpp"

void GUIView::showDiceResult(int d1, int d2, const string& playerName) {
    lastD1_ = d1;
    lastD2_ = d2;
    diceRolled_ = true;
    (void)playerName;
}

void GUIView::showPlayerLanding(const string& playerName, const string& tileName) {
    (void)playerName;
    (void)tileName;
}

void GUIView::showPropertyCard(const PropertyInfo& propertyInfo) {
    (void)propertyInfo;
}

void GUIView::showPlayerProperties(const vector<PropertyInfo>& list) {
    (void)list;
}

void GUIView::showBuyPrompt(const PropertyInfo& propertyInfo, Money playerMoney) {
    (void)propertyInfo;
    (void)playerMoney;
}

void GUIView::showRentPayment(const RentInfo& rentInfo) {
    (void)rentInfo;
}

void GUIView::showTaxPrompt(const TaxInfo& taxInfo) {
    (void)taxInfo;
}

void GUIView::showAuctionState(const AuctionState& auctionState) {
    (void)auctionState;
}

void GUIView::showFestivalPrompt(const vector<PropertyInfo>& ownedProperties) {
    (void)ownedProperties;
}

void GUIView::showBankruptcy(const BankruptcyInfo& bankruptcyInfo) {
    (void)bankruptcyInfo;
}

void GUIView::showLiquidationPanel(const LiquidationState& liquidationState) {
    (void)liquidationState;
}

void GUIView::showCardDrawn(const CardInfo& cardInfo) {
    (void)cardInfo;
}

void GUIView::showSkillCardHand(const vector<CardInfo>& cards) {
    (void)cards;
}

void GUIView::showTransactionLog(const vector<LogEntry>& entries) {
    log_ = entries;
}

void GUIView::showWinner(const WinnerInfo& winInfo) {
    (void)winInfo;
}

void GUIView::showJailStatus(const JailInfo& jailInfo) {
    (void)jailInfo;
}

void GUIView::showMessage(const string& message) {
    (void)message;
}

void GUIView::showBuildMenu(const BuildMenuState& buildMenuState) {
    (void)buildMenuState;
}

void GUIView::showMortgageMenu(const MortgageMenuState& mortgageMenuState) {
    (void)mortgageMenuState;
}

void GUIView::showRedeemMenu(const RedeemMenuState& redeemMenuState) {
    (void)redeemMenuState;
}

void GUIView::showDropCardPrompt(const vector<CardInfo>& cards) {
    (void)cards;
}

void GUIView::showSaveLoadStatus(const string& message) {
    (void)message;
}

void GUIView::showTurnInfo(const string& playerName, int turnNum, int maxTurn) {
    diceRolled_ = false;
    (void)playerName; (void)turnNum; (void)maxTurn;
}

void GUIView::showMainMenu() {}

void GUIView::showPlayerOrder(const vector<string>& orderedNames) {
    (void)orderedNames;
}

void GUIView::showDoubleBonusTurn(const string& playerName, int doubleCount) {
    (void)playerName;
    (void)doubleCount;
}

void GUIView::showAuctionWinner(const AuctionSummary& summary) {
    (void)summary;
}

void GUIView::showFestivalReinforced(const FestivalEffectInfo& info) {
    (void)info;
}

void GUIView::showFestivalAtMax(const FestivalEffectInfo& info) {
    (void)info;
}

void GUIView::showJailEntry(const JailEntryInfo& info) {
    (void)info;
}

void GUIView::showCardEffect(const CardEffectInfo& info) {
    (void)info;
}

void GUIView::showLiquidationResult(bool canCover, Money finalBalance) {
    (void)canCover;
    (void)finalBalance;
}
