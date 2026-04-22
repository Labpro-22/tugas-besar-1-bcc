#ifndef TILE_HEADER_TAX_TILE_HPP
#define TILE_HEADER_TAX_TILE_HPP

#include "tile/header/ActionTile.hpp"
#include "models/Money.hpp"
#include "utils/Enums.hpp"

class TaxTile : public ActionTile {
public:
	TaxTile(int id, const std::string& code, const std::string& name, TileType type);

	virtual Money calculateTax(const class Player& player) const = 0;
};

#endif
