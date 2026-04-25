#ifndef TILE_HEADER_RAILROAD_TILE_HPP
#define TILE_HEADER_RAILROAD_TILE_HPP

#include <vector>

#include "tile/header/PropertyTile.hpp"
#include "utils/Enums.hpp"

class RailroadTile : public PropertyTile {
public:
	RailroadTile(int id, const std::string& code, const std::string& name,
				 Money price, Money mortgageValue, const std::vector<int>& rentTable);

	Money getRent(int diceRoll = 0) const override;

private:
	std::vector<int> rentTable;
};

#endif
