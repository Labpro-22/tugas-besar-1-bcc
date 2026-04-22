#ifndef TILE_HEADER_PPH_TILE_HPP
#define TILE_HEADER_PPH_TILE_HPP

#include "tile/header/TaxTile.hpp"
#include "models/Money.hpp"
#include "utils/Enums.hpp"

class PPHTile : public TaxTile {
public:
	PPHTile(int id, const std::string& code, const std::string& name, int flatAmount, int percentage);

	Money calculateTax(const Player& player) const override;

	int getFlatAmount() const;
	int getPercentage() const;

private:
	int flatAmount;
	int percentage;
};

#endif
