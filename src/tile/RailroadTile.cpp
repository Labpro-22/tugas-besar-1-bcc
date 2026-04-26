#include "tile/header/RailroadTile.hpp"

#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "utils/Types.hpp"
#include "core/log/header/TransactionLogger.hpp"

RailroadTile::RailroadTile(int id, const std::string& code, const std::string& name,
							   Money price, Money mortgageValue, const std::vector<int>& rentValues)
		: PropertyTile(id, code, name, TileType::RAILROAD, price, mortgageValue), rentTable(rentValues) {}

Money RailroadTile::getRent(int /*diceRoll*/) const {
	if (isMortgaged() || !getOwner())
		return Money::zero();

	const int count = getOwner()->countRailroads();
	int rent = 0;
	if (count > 0) {
		if (count < static_cast<int>(rentTable.size()) && rentTable[static_cast<std::size_t>(count)] > 0) {
			rent = rentTable[static_cast<std::size_t>(count)];
		} else {
			for (int i = static_cast<int>(rentTable.size()) - 1; i >= 1; --i) {
				if (rentTable[static_cast<std::size_t>(i)] > 0) {
					rent = rentTable[static_cast<std::size_t>(i)];
					break;
				}
			}
		}
	}
	return Money(rent);
}

void RailroadTile::onLand(Player& player, GameContext& ctx, int diceTotal) {
	PropertyTile::onLand(player, ctx, diceTotal);
}
