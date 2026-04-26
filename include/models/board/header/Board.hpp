#ifndef MODELS_BOARD_HEADER_BOARD_HPP
#define MODELS_BOARD_HEADER_BOARD_HPP

#include <memory>
#include <vector>
#include <string>

#include "models/board/header/ColorGroup.hpp"
#include "tile/header/Tile.hpp"
#include "utils/Enums.hpp"
#include "ui/UiDtos.hpp"

class StreetTile;
class PropertyTile;
class Player;

class Board {
public:
	Board();
	~Board();

	Board(const Board& other) = delete;
	Board& operator=(const Board& other) = delete;
	Board(Board&& other) noexcept = default;
	Board& operator=(Board&& other) noexcept = default;

	void addTile(std::unique_ptr<Tile> tile);
	void clear();
	Tile* getTile(int index) const;
	Tile* getTileByCode(const std::string& code) const;
	int getSize() const;

	StreetTile* getStreetByCode(const std::string& code) const;
	PropertyTile* getPropertyByCode(const std::string& code) const;

	void registerColorGroup(ColorGroup* group);
	ColorGroup* ensureColorGroup(Color color);
	ColorGroup* getColorGroup(Color color) const;
	void updateMonopolies();
	BuildMenuState getBuildMenuState(const Player& player) const;

private:
	std::vector<std::unique_ptr<Tile>> tiles;
	std::vector<std::unique_ptr<ColorGroup>> ownedColorGroups;
	std::vector<ColorGroup*> colorGroups;
};

#endif
