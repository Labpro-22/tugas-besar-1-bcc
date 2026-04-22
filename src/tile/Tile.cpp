#include "tile/header/Tile.hpp"

Tile::Tile(int id, const std::string& code, const std::string& name, TileType type)
	: id(id), code(code), name(name), type(type) {}

int Tile::getId() const {
	return id;
}

const std::string& Tile::getCode() const {
	return code;
}

const std::string& Tile::getName() const {
	return name;
}

TileType Tile::getType() const {
	return type;
}

void Tile::onLand(Player& /*player*/, GameContext& /*ctx*/) {
	// Base tile does nothing on land
}
