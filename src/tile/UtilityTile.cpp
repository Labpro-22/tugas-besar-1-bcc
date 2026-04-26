#include "tile/header/UtilityTile.hpp"

#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "utils/Types.hpp"
#include "core/log/header/TransactionLogger.hpp"

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
	PropertyTile::onLand(player, ctx, diceTotal);
}
