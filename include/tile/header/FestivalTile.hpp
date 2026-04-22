#ifndef TILE_HEADER_FESTIVAL_TILE_HPP
#define TILE_HEADER_FESTIVAL_TILE_HPP

#include "tile/header/ActionTile.hpp"
#include "utils/Enums.hpp"

class FestivalTile : public ActionTile {
public:
	FestivalTile(int id, const std::string& code, const std::string& name);
};

#endif
