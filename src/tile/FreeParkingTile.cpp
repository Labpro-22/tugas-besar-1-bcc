#include "tile/header/FreeParkingTile.hpp"

FreeParkingTile::FreeParkingTile(int id, const std::string& code, const std::string& name)
	: SpecialTile(id, code, name, TileType::FREE_PARKING) {}
 