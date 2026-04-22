#include "tile/header/FestivalTile.hpp"

FestivalTile::FestivalTile(int id, const std::string& code, const std::string& name)
	: ActionTile(id, code, name, TileType::FESTIVAL) {}
