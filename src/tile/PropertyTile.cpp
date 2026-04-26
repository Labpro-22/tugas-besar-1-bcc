#include "tile/header/PropertyTile.hpp"

#include <algorithm>

#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "tile/header/StreetTile.hpp"
#include "ui/UiDtos.hpp"
#include "utils/Types.hpp"
#include "controllers/PlayerController.hpp"
#include "core/Bank.hpp"
#include "core/FestivalManager.hpp"
#include "core/log/header/TransactionLogger.hpp"
#include "manager/header/AuctionManager.hpp"
#include "manager/header/BankruptcyManager.hpp"

PropertyTile::PropertyTile(int id, const std::string& code, const std::string& name,
						   TileType type, Money price, Money mortgageValue)
	: Tile(id, code, name, type), price(price), mortgageValue(mortgageValue),
	  owner(nullptr), status(PropertyStatus::BANK) {}

Money PropertyTile::getPrice() const {
	return price;
}

Money PropertyTile::getMortgageValue() const {
	return mortgageValue;
}

Player* PropertyTile::getOwner() const {
	return owner;
}

void PropertyTile::setOwner(Player* player) {
	owner = player;
}

PropertyStatus PropertyTile::getStatus() const {
	return status;
}

void PropertyTile::setStatus(PropertyStatus s) {
	status = s;
}

bool PropertyTile::isOwned() const {
	return status == PropertyStatus::OWNED;
}

bool PropertyTile::isMortgaged() const {
	return status == PropertyStatus::MORTGAGED;
}

bool PropertyTile::canBeDeveloped() const {
	return false;
}

int PropertyTile::getBuildingLevel() const {
	return 0;
}

std::string PropertyTile::getLabel() const {
	return getName() + " (" + getCode() + ")";
}

PropertyInfo PropertyTile::toInfo() const {
	PropertyInfo info;
	info.code = getCode();
	info.name = getName();
	info.ownerName = getOwner() ? getOwner()->getUsername() : "";
	info.status = getStatus();
	info.buildingLevel = getBuildingLevel();
	info.purchasePrice = getPrice();
	return info;
}

Money PropertyTile::getSaleValue() const {
	if (isMortgaged()) {
		return Money::zero();
	}
	int total = getPrice().getAmount();
	if (const auto* street = dynamic_cast<const StreetTile*>(this)) {
		total += street->getBuildingSaleRefund();
	}
	return Money(total);
}

bool PropertyTile::canMortgageNow(const Board& board) const {
	if (isMortgaged()) {
		return false;
	}
	const auto* street = dynamic_cast<const StreetTile*>(this);
	if (!street) {
		return true;
	}
	const ColorGroup* group = board.getColorGroup(street->getColor());
	if (!group) {
		return street->getBuildingLevel() == 0;
	}
	for (const StreetTile* member : group->getStreets()) {
		if (member && member->getBuildingLevel() > 0) {
			return false;
		}
	}
	return true;
}

void PropertyTile::onLand(Player& player, GameContext& ctx, int diceTotal) {
	if (!getOwner()) {
		bool wantsBuy = false;
		if (PlayerController* controller = player.getController()) {
			wantsBuy = controller->decideBuyProperty(toInfo(), player.getMoney());
		}
		if (wantsBuy && player.canAfford(getPrice())) {
			ctx.bank.collectFromPlayer(player, getPrice(), "Beli properti");
			setOwner(&player);
			setStatus(PropertyStatus::OWNED);
			player.addProperty(this);
			ctx.board.updateMonopolies();
			ctx.logger.log(ctx.currentTurn, player.getUsername(), "BELI",
				"Beli " + getLabel() + " " + getPrice().toString());
		} else {
			ctx.auctionManager->runAuction(*this, player, ctx.players, ctx.bank, ctx.board, ctx.logger, ctx.currentTurn);
		}
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
	rent = owner->applyIncomingModifiers(rent, this);
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
