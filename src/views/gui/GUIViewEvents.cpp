#include "ui/GUIView.hpp"

#include <cstdio>
#include <sstream>

void GUIView::startGameTimer() {
    gameStartedAt_ = std::chrono::steady_clock::now();
    gameTimerRunning_ = true;
}

std::string GUIView::formattedElapsedTime() const {
    if (!gameTimerRunning_) {
        return "00:00:00";
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - gameStartedAt_).count();
    const long long hours = elapsed / 3600;
    const long long minutes = (elapsed % 3600) / 60;
    const long long seconds = elapsed % 60;

    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld:%02lld", hours, minutes, seconds);
    return buffer;
}

void GUIView::showDiceResult(int d1, int d2, const string& playerName) {
    lastD1_ = d1;
    lastD2_ = d2;
    showSaveLoadStatus(playerName + " melempar dadu: " + std::to_string(d1) + " + " + std::to_string(d2));
}

void GUIView::showPlayerLanding(const string& playerName, const string& tileName) {
    showSaveLoadStatus(playerName + " mendarat di " + tileName + ".");
}

void GUIView::showPropertyCard(const PropertyInfo& propertyInfo) {
    showSaveLoadStatus("Akta: " + propertyInfo.name + " (" + propertyInfo.code + ")");
}

void GUIView::showPlayerProperties(const vector<PropertyInfo>& list) {
    showSaveLoadStatus("Jumlah properti aktif: " + std::to_string(list.size()));
}

void GUIView::showBuyPrompt(const PropertyInfo& propertyInfo, Money playerMoney) {
    buyPromptInfo_ = propertyInfo;
    buyPromptMoney_ = playerMoney;
    buyPromptActive_ = true;
}

void GUIView::showRentPayment(const RentInfo& rentInfo) {
    showSaveLoadStatus(rentInfo.payerName + " bayar sewa " + rentInfo.amount.toString() +
                       " ke " + rentInfo.ownerName + ".");
}

void GUIView::showTaxPrompt(const TaxInfo& taxInfo) {
    if (taxInfo.taxType == TileType::TAX_PPH) {
        showSaveLoadStatus("Pilih pajak PPH: flat atau persentase.");
    } else {
        showSaveLoadStatus("Bayar pajak PBM: " + std::to_string(taxInfo.flatAmount));
    }
}

void GUIView::showAuctionState(const AuctionState& auctionState) {
    std::string bidder = auctionState.currentBidderName.empty() ? "-" : auctionState.currentBidderName;
    showSaveLoadStatus("Lelang " + auctionState.property.code + " | Bidder: " + bidder +
                       " | Bid: " + std::to_string(auctionState.currentBid));
}

void GUIView::showFestivalPrompt(const vector<PropertyInfo>& ownedProperties) {
    showSaveLoadStatus("Festival aktif. Pilih properti (" + std::to_string(ownedProperties.size()) + " opsi).");
}

void GUIView::showBankruptcy(const BankruptcyInfo& bankruptcyInfo) {
    const string creditor = bankruptcyInfo.creditorName.empty()
                                ? string("Bank")
                                : bankruptcyInfo.creditorName;
    showSaveLoadStatus(bankruptcyInfo.playerName + " bangkrut. Kreditor: " + creditor + ".");
}

void GUIView::showLiquidationPanel(const LiquidationState& liquidationState) {
    showSaveLoadStatus("Likuidasi: kewajiban " + liquidationState.obligation.toString() +
                       ", opsi " + std::to_string(liquidationState.options.size()) + ".");
}

void GUIView::showCardDrawn(const CardInfo& cardInfo) {
    showSaveLoadStatus("Kartu: " + cardInfo.name + " - " + cardInfo.description);
}

void GUIView::showSkillCardHand(const vector<CardInfo>& cards) {
    showSaveLoadStatus("Kartu kemampuan di tangan: " + std::to_string(cards.size()));
}

void GUIView::showTransactionLog(const vector<LogEntry>& entries) {
    log_ = entries;
}

void GUIView::showWinner(const WinnerInfo& winInfo) {
    winnerInfo_ = winInfo;
}

void GUIView::showJailStatus(const JailInfo& jailInfo) {
    showSaveLoadStatus(jailInfo.playerName + " di penjara. Sisa percobaan: " +
                       std::to_string(jailInfo.escapeAttemptsLeft));
}

void GUIView::showMessage(const string& message) {
    showSaveLoadStatus(message);
}

void GUIView::showBuildMenu(const BuildMenuState& buildMenuState) {
    showSaveLoadStatus("Menu bangun tersedia: " + std::to_string(buildMenuState.groups.size()) + " grup warna.");
}

void GUIView::showMortgageMenu(const MortgageMenuState& mortgageMenuState) {
    showSaveLoadStatus("Menu gadai tersedia: " + std::to_string(mortgageMenuState.options.size()) + " opsi.");
}

void GUIView::showRedeemMenu(const RedeemMenuState& redeemMenuState) {
    showSaveLoadStatus("Menu tebus tersedia: " + std::to_string(redeemMenuState.options.size()) + " opsi.");
}

void GUIView::showDropCardPrompt(const vector<CardInfo>& cards) {
    showSaveLoadStatus("Pilih kartu yang dibuang (" + std::to_string(cards.size()) + " kartu).");
}

void GUIView::showSaveLoadStatus(const string& message) {
    saveLoadStatus_ = message;
    saveLoadStatusFrames_ = 180; // 3 seconds at 60fps
}

void GUIView::showTurnInfo(const string& playerName, int turnNum, int maxTurn) {
    showSaveLoadStatus("Turn global " + std::to_string(turnNum) +
                       " | batas per pemain " + std::to_string(maxTurn) +
                       " - " + playerName);
}

void GUIView::showMainMenu() {}

void GUIView::showPlayerOrder(const vector<string>& orderedNames) {
    if (orderedNames.empty()) {
        return;
    }
    showSaveLoadStatus("Urutan main telah ditentukan.");
}

void GUIView::showDoubleBonusTurn(const string& playerName, int doubleCount) {
    showSaveLoadStatus(playerName + " mendapat bonus roll (double ke-" + std::to_string(doubleCount) + ").");
}

void GUIView::showAuctionWinner(const AuctionSummary& summary) {
    if (summary.noWinner) {
        showSaveLoadStatus("Lelang " + summary.property.code + " selesai tanpa pemenang.");
        return;
    }
    showSaveLoadStatus("Pemenang lelang " + summary.property.code + ": " + summary.winnerName +
                       " (" + std::to_string(summary.finalBid) + ")");
}

void GUIView::showFestivalReinforced(const FestivalEffectInfo& info) {
    showSaveLoadStatus("Festival diperkuat di " + info.propertyCode +
                       " x" + std::to_string(info.newMultiplier));
}

void GUIView::showFestivalAtMax(const FestivalEffectInfo& info) {
    showSaveLoadStatus("Festival di " + info.propertyCode + " sudah maksimum.");
}

void GUIView::showJailEntry(const JailEntryInfo& info) {
    showSaveLoadStatus(info.playerName + " masuk penjara.");
}

void GUIView::showCardEffect(const CardEffectInfo& info) {
    showSaveLoadStatus("Efek kartu " + info.cardType + ": " + info.detail);
}

void GUIView::showLiquidationResult(bool canCover, Money finalBalance) {
    if (canCover) {
        showSaveLoadStatus("Likuidasi berhasil. Sisa saldo: " + finalBalance.toString());
    } else {
        showSaveLoadStatus("Likuidasi gagal menutup kewajiban.");
    }
}
