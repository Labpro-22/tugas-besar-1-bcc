#include "tile/header/TaxTile.hpp"

TaxTile::TaxTile(int id, const std::string& code, const std::string& name, TileType type)
	: ActionTile(id, code, name, type) {}
