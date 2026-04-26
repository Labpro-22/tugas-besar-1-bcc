#include "controllers/HumanController.hpp"

#include "ui/IGameView.hpp"

HumanController::HumanController(IGameInput* input, IGameView* view, const string& playerName)
    : input(input), view_(view), playerName_(playerName) {}

string HumanController::chooseCommand(const GameStateView& state) {
    (void)state;
    return input->getCommand();
}

bool HumanController::decideBuyProperty(const PropertyInfo& info, Money money) {
    if (view_) {
        view_->showBuyPrompt(info, money);
    }
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
    return input->getSkillCardChoice(cards);
}

string HumanController::decideFestivalProperty(const vector<PropertyInfo>& props) {
    if (props.empty()) {
        return "";
    }
    vector<string> options;
    for (const auto& prop : props) {
        options.push_back(prop.code + " - " + prop.name);
    }
    int choice = input->getMenuChoice(options);
    if (choice > 0 && choice <= static_cast<int>(props.size())) {
        return props[static_cast<size_t>(choice - 1)].code;
    }
    return "";
}

int HumanController::decideBuild(const BuildMenuState& state) {
    vector<string> options;
    for (const auto& g : state.groups) {
        options.push_back(g.colorName);
    }
    return input->getMenuChoice(options);
}

int HumanController::decideLiquidation(const LiquidationState& state) {
    return input->getLiquidationChoice(state);
}

int HumanController::decideDropCard(const vector<CardInfo>& cards) {
    if (view_) {
        view_->showDropCardPrompt(cards);
    }
    return input->getSkillCardChoice(cards);
}

bool HumanController::decideJailPay() {
    return input->getYesNo("Bayar denda penjara? (y/n)");
}

bool HumanController::confirmAction(const std::string& prompt) {
    return input->getYesNo(prompt);
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
