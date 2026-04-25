#ifndef TILE_HEADER_UTILITY_TILE_HPP
#define TILE_HEADER_UTILITY_TILE_HPP

#include <vector>

#include "tile/header/PropertyTile.hpp"
#include "utils/Enums.hpp"

class UtilityTile : public PropertyTile {
public:
	UtilityTile(int id, const std::string& code, const std::string& name,
					Money price, Money mortgageValue, const std::vector<int>& multiplierTable);

	Money getRent(int diceRoll) const override;

private:
	std::vector<int> multiplierTable;
};

#endif
