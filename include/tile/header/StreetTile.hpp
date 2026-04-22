#ifndef TILE_HEADER_STREET_TILE_HPP
#define TILE_HEADER_STREET_TILE_HPP

#include <vector>
#include "tile/header/PropertyTile.hpp"
#include "utils/Enums.hpp"

class StreetTile : public PropertyTile {
public:
	StreetTile(int id, const std::string& code, const std::string& name,
			   Money price, Money mortgageValue, Color color,
			   Money houseCost, Money hotelCost,
			   const std::vector<int>& rentLevels);

	Color getColor() const;
	Money getHouseCost() const;
	Money getHotelCost() const;

	int getBuildingLevel() const override;
	bool hasHotel() const;

	bool canBuild() const;
	void build();
	void demolish();

	Money getRent(int diceRoll = 0) const override;
	bool canBeDeveloped() const override;

	bool isMonopolyComplete() const;
	void setMonopolyComplete(bool value);

private:
	Color color;
	Money houseCost;
	Money hotelCost;
	std::vector<int> rentLevels;
	int buildingLevel;
	bool monopolyComplete;
};

#endif
