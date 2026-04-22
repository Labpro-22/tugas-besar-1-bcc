#include "core/Bank.hpp"
#include "models/Player.hpp"
#include "utils/Exceptions.hpp"

namespace {
    Money getPayableAmount(const Player& player, const Money& amount) {
        if (player.isPaymentBlocked()) {
            return Money::zero();
        }
        return player.applyOutgoingModifiers(amount);
    }
}

void Bank::payPlayer(Player& player, const Money& amount, const std::string& /*reason*/) {
    player.addMoney(amount);
}

void Bank::collectFromPlayer(Player& player, const Money& amount, const std::string& /*reason*/) {
    Money payableAmount = getPayableAmount(player, amount);
    if (payableAmount.isZero()) {
        return;
    }

    if (!player.canAfford(payableAmount))
        throw InsufficientFundsException(payableAmount, player.getMoney());
    player.deductMoney(payableAmount);
}

void Bank::transferBetweenPlayers(Player& from, Player& to, const Money& amount, const std::string& /*reason*/) {
    Money payableAmount = getPayableAmount(from, amount);
    if (payableAmount.isZero()) {
        return;
    }

    if (!from.canAfford(payableAmount))
        throw InsufficientFundsException(payableAmount, from.getMoney());
    from.deductMoney(payableAmount);
    to.addMoney(payableAmount);
}
