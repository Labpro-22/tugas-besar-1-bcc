#include "tile/header/StreetTile.hpp"

StreetTile::StreetTile(int id, const std::string& code, const std::string& name,
					   Money price, Money mortgageValue, Color color,
					   Money houseCost, Money hotelCost,
					   const std::vector<int>& rentLevels)
	: PropertyTile(id, code, name, TileType::STREET, price, mortgageValue),
	  color(color), houseCost(houseCost), hotelCost(hotelCost),
	  rentLevels(rentLevels), buildingLevel(0), monopolyComplete(false) {}

Color StreetTile::getColor() const {
	return color;
}

Money StreetTile::getHouseCost() const {
	return houseCost;
}

Money StreetTile::getHotelCost() const {
	return hotelCost;
}

int StreetTile::getBuildingLevel() const {
	return buildingLevel;
}

bool StreetTile::hasHotel() const {
	return buildingLevel == 5;
}

bool StreetTile::canBuild() const {
	if (!monopolyComplete || isMortgaged())
		return false;
	if (buildingLevel < 4)
		return true;
	if (buildingLevel == 4)
		return true; // Can build hotel
	return false;
}

void StreetTile::build() {
	if (!canBuild())
		return;
	++buildingLevel;
}

void StreetTile::demolish() {
	if (buildingLevel > 0)
		--buildingLevel;
}

Money StreetTile::getRent(int /*diceRoll*/) const {
	if (isMortgaged())
		return Money::zero();

	int level = buildingLevel;
	if (level >= 0 && level < static_cast<int>(rentLevels.size())) {
		int rent = rentLevels[level];
		if (level == 0 && monopolyComplete)
			rent *= 2;
		return Money(rent);
	}
	return Money::zero();
}

bool StreetTile::canBeDeveloped() const {
	return true;
}

bool StreetTile::isMonopolyComplete() const {
	return monopolyComplete;
}

void StreetTile::setMonopolyComplete(bool value) {
	monopolyComplete = value;
}
