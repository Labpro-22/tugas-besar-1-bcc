#ifndef TILE_HEADER_PBM_TILE_HPP
#define TILE_HEADER_PBM_TILE_HPP

#include "tile/header/TaxTile.hpp"
#include "models/Money.hpp"
#include "utils/Enums.hpp"

class PBMTile : public TaxTile {
public:
	PBMTile(int id, const std::string& code, const std::string& name, int flatAmount);

	Money calculateTax(const Player& player) const override;

	int getFlatAmount() const;

private:
	int flatAmount;
};

#endif
