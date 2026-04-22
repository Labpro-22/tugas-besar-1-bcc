#include "controllers/HumanController.hpp"

HumanController::HumanController(IGameInput* input, const string& playerName)
    : input(input), playerName_(playerName) {}

string HumanController::chooseCommand(const GameStateView& state) {
    (void)state;
    return input->getCommand();
}

bool HumanController::decideBuyProperty(const PropertyInfo& info, Money money) {
    (void)info;
    (void)money;
    return input->getYesNo("Apakah kamu ingin membeli properti ini? (y/n)");
}

AuctionDecision HumanController::decideAuction(int currentBid, Money money) {
    return input->getAuctionDecision(playerName_, currentBid, money.getAmount());
}

TaxChoice HumanController::decideTax(int flatAmount, int percentAmount) {
    (void)flatAmount;
    (void)percentAmount;
    return input->getTaxChoice();
}

int HumanController::decideSkillCard(const vector<CardInfo>& cards) {
    return input->getSkillCardChoice(static_cast<int>(cards.size()));
}

string HumanController::decideFestivalProperty(const vector<PropertyInfo>& props) {
    (void)props;
    return input->getPropertyCodeInput("Masukkan kode properti");
}

int HumanController::decideBuild(const BuildMenuState& state) {
    vector<string> options;
    for (const auto& g : state.groups) {
        options.push_back(g.colorName);
    }
    return input->getMenuChoice(options);
}

int HumanController::decideLiquidation(const LiquidationState& state) {
    return input->getLiquidationChoice(static_cast<int>(state.options.size()));
}

int HumanController::decideDropCard(const vector<CardInfo>& cards) {
    return input->getSkillCardChoice(static_cast<int>(cards.size()));
}

bool HumanController::decideJailPay() {
    return input->getYesNo("Bayar denda penjara? (y/n)");
}

string HumanController::decideTeleportTarget() {
    return input->getPropertyCodeInput("Masukkan kode petak tujuan");
}

string HumanController::decideLassoTarget(const vector<string>& players) {
    (void)players;
    return input->getString("Masukkan nama pemain target Lasso");
}

string HumanController::decideDemolitionTarget(const vector<PropertyInfo>& properties) {
    (void)properties;
    return input->getPropertyCodeInput("Masukkan kode properti target Demolition");
}
