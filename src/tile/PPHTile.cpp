#include "tile/header/PPHTile.hpp"
#include "models/Player.hpp"

PPHTile::PPHTile(int id, const std::string& code, const std::string& name,
				 int flatAmount, int percentage)
	: TaxTile(id, code, name, TileType::TAX_PPH),
	  flatAmount(flatAmount), percentage(percentage) {}

Money PPHTile::calculateTax(const Player& /*player*/) const {
	// Tax calculation is done in the game engine based on player choice
	return Money::zero();
}

int PPHTile::getFlatAmount() const {
	return flatAmount;
}

int PPHTile::getPercentage() const {
	return percentage;
}
