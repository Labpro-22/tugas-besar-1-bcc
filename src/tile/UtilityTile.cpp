#include "tile/header/UtilityTile.hpp"

#include "core/Bank.hpp"
#include "core/FestivalManager.hpp"
#include "core/log/header/TransactionLogger.hpp"
#include "manager/header/AuctionManager.hpp"
#include "manager/header/BankruptcyManager.hpp"
#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "utils/Types.hpp"

UtilityTile::UtilityTile(int id, const std::string& code, const std::string& name,
						 Money price, Money mortgageValue, const std::vector<int>& multipliers)
		: PropertyTile(id, code, name, TileType::UTILITY, price, mortgageValue), multiplierTable(multipliers) {}

Money UtilityTile::getRent(int diceRoll) const {
	if (isMortgaged() || !getOwner() || diceRoll <= 0)
		return Money::zero();

	const int count = getOwner()->countUtilities();
	int factor = 0;
	if (count > 0) {
		if (count < static_cast<int>(multiplierTable.size()) &&
			multiplierTable[static_cast<std::size_t>(count)] > 0) {
			factor = multiplierTable[static_cast<std::size_t>(count)];
		} else {
			for (int i = static_cast<int>(multiplierTable.size()) - 1; i >= 1; --i) {
				if (multiplierTable[static_cast<std::size_t>(i)] > 0) {
					factor = multiplierTable[static_cast<std::size_t>(i)];
					break;
				}
			}
		}
	}
	return Money(diceRoll * factor);
}

void UtilityTile::onLand(Player& player, GameContext& ctx, int diceTotal) {
	if (!getOwner()) {
		setOwner(&player);
		setStatus(PropertyStatus::OWNED);
		player.addProperty(this);
		ctx.board.updateMonopolies();
		ctx.logger.log(ctx.currentTurn, player.getUsername(), "BELI",
			"Mendapat " + getLabel() + " gratis");
		std::cout << player.getUsername() << " mendapat " << getName() << " secara otomatis.\n";
		return;
	}
	if (getOwner() == &player || isMortgaged()) {
		return;
	}
	Player* owner = getOwner();
	Money rent = getRent(diceTotal);
	const int festivalMult = ctx.festivalManager.getMultiplier(this);
	if (festivalMult > 1) {
		rent = Money(rent.getAmount() * festivalMult);
	}
	if (!player.canAfford(player.applyOutgoingModifiers(rent))) {
		std::cout << player.getUsername() << " tidak mampu membayar sewa " << rent.toString() << ".\n";
		ctx.bankruptcyManager->execute(player, owner, rent, *ctx.gameState, ctx.board, ctx.bank,
			ctx.festivalManager, ctx.logger, *ctx.turnManager, *ctx.auctionManager, *ctx.eventBus);
		return;
	}
	ctx.bank.transferBetweenPlayers(player, *owner, rent, "Sewa " + getCode());
	ctx.logger.log(ctx.currentTurn, player.getUsername(), "SEWA",
		"Bayar sewa " + getLabel() + " " + rent.toString());
}
