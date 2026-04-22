#include "tile/header/PBMTile.hpp"

PBMTile::PBMTile(int id, const std::string& code, const std::string& name, int flatAmount)
	: TaxTile(id, code, name, TileType::TAX_PBM), flatAmount(flatAmount) {}

Money PBMTile::calculateTax(const Player& /*player*/) const {
	return Money(flatAmount);
}

int PBMTile::getFlatAmount() const {
	return flatAmount;
}
