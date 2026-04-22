#ifndef TILE_HEADER_TILE_HPP
#define TILE_HEADER_TILE_HPP

#include <string>
#include "utils/Enums.hpp"

class Tile {
public:
	Tile(int id, const std::string& code, const std::string& name, TileType type);
	virtual ~Tile() = default;

	int getId() const;
	const std::string& getCode() const;
	const std::string& getName() const;
	TileType getType() const;

	virtual void onLand(class Player& player, class GameContext& ctx);

private:
	int id;
	std::string code;
	std::string name;
	TileType type;
};

#endif
