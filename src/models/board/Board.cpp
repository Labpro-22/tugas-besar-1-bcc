#include "models/board/header/Board.hpp"
#include "tile/header/Tile.hpp"
#include "tile/header/StreetTile.hpp"
#include "tile/header/PropertyTile.hpp"
#include "models/board/header/ColorGroup.hpp"
#include "models/Player.hpp"
#include "utils/GameUtils.hpp"

Board::Board() = default;
Board::~Board() = default;

void Board::addTile(std::unique_ptr<Tile> tile) {
	tiles.push_back(std::move(tile));
}

void Board::clear() {
	tiles.clear();
	colorGroups.clear();
	ownedColorGroups.clear();
}

Tile* Board::getTile(int index) const {
	if (index < 0 || index >= static_cast<int>(tiles.size()))
		return nullptr;
	return tiles[index].get();
}

Tile* Board::getTileByCode(const std::string& code) const {
	for (const auto& tile : tiles) {
		if (tile->getCode() == code)
			return tile.get();
	}
	return nullptr;
}

int Board::getSize() const {
	return static_cast<int>(tiles.size());
}

StreetTile* Board::getStreetByCode(const std::string& code) const {
	Tile* tile = getTileByCode(code);
	if (!tile || tile->getType() != TileType::STREET)
		return nullptr;
	return static_cast<StreetTile*>(tile);
}

PropertyTile* Board::getPropertyByCode(const std::string& code) const {
	Tile* tile = getTileByCode(code);
	if (!tile)
		return nullptr;
	switch (tile->getType()) {
		case TileType::STREET:
		case TileType::RAILROAD:
		case TileType::UTILITY:
			return static_cast<PropertyTile*>(tile);
		default:
			return nullptr;
	}
}

void Board::registerColorGroup(ColorGroup* group) {
	colorGroups.push_back(group);
}

ColorGroup* Board::ensureColorGroup(Color color) {
	if (ColorGroup* existing = getColorGroup(color)) {
		return existing;
	}

	ownedColorGroups.push_back(std::make_unique<ColorGroup>(color));
	ColorGroup* group = ownedColorGroups.back().get();
	registerColorGroup(group);
	return group;
}

ColorGroup* Board::getColorGroup(Color color) const {
	for (auto* group : colorGroups) {
		if (group->getColor() == color)
			return group;
	}
	return nullptr;
}

void Board::updateMonopolies() {
	for (auto* group : colorGroups) {
		bool monopolized = group->isMonopolized();
		for (auto* street : group->getStreets()) {
			street->setMonopolyComplete(monopolized);
		}
	}
}

BuildMenuState Board::getBuildMenuState(const Player& player) const {
	BuildMenuState state;
	state.playerMoney = player.getMoney();

	for (PropertyTile* property : player.getProperties()) {
		if (!property || property->isMortgaged()) continue;
		StreetTile* street = dynamic_cast<StreetTile*>(property);
		if (!street || !street->isMonopolyComplete()) continue;
		if (street->getBuildingLevel() >= 5) continue;

		ColorGroup* group = getColorGroup(street->getColor());
		if (!group || !group->satisfiesEvenBuildRule(*street)) continue;

		TileBuildOption option;
		option.code = street->getCode();
		option.name = street->getName();
		option.currentLevel = street->getBuildingLevel();
		option.canBuild = true;
		option.buildCost = street->getBuildingLevel() < 4
			? street->getHouseCost().getAmount()
			: street->getHotelCost().getAmount();

		bool found = false;
		for (ColorGroupBuildOption& groupOpt : state.groups) {
			if (groupOpt.color == street->getColor()) {
				groupOpt.tiles.push_back(option);
				found = true;
				break;
			}
		}
		if (!found) {
			ColorGroupBuildOption groupOpt;
			groupOpt.colorName = colorName(street->getColor());
			groupOpt.color = street->getColor();
			groupOpt.tiles.push_back(option);
			state.groups.push_back(groupOpt);
		}
	}

	return state;
}
