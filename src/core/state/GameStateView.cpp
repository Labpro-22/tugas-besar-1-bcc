#include "core/state/header/GameStateView.hpp"

#include <map>

#include "core/FestivalEffectSnapshot.hpp"
#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "tile/header/PropertyTile.hpp"
#include "tile/header/RailroadTile.hpp"
#include "tile/header/Tile.hpp"
#include "tile/header/StreetTile.hpp"
#include "tile/header/UtilityTile.hpp"

namespace {
	std::string tileSubtitle(const Tile& tile) {
		switch (tile.getType()) {
			case TileType::JAIL: return "Just Visiting";
			default: return "";
		}
	}

	void populatePropertyView(PropertyView& snapshot,
		const PropertyTile& property,
		int festivalMultiplier,
		int festivalTurnsRemaining) {
		snapshot.code = property.getCode();
		snapshot.name = property.getName();
		snapshot.ownerName = property.getOwner() ? property.getOwner()->getUsername() : "";
		snapshot.status = property.getStatus();
		snapshot.type = property.getType();
		snapshot.purchasePrice = property.getPrice().getAmount();
		snapshot.mortgageValue = property.getMortgageValue().getAmount();
		snapshot.buildingLevel = property.getBuildingLevel();
		snapshot.isMortgaged = property.isMortgaged();
		snapshot.monopolyComplete = false;
		snapshot.colorGroup = Color::DEFAULT;
		if (const auto* street = dynamic_cast<const StreetTile*>(&property)) {
			snapshot.colorGroup = street->getColor();
			snapshot.monopolyComplete = street->isMonopolyComplete();
		}
		snapshot.festivalMultiplier = festivalMultiplier;
		snapshot.festivalTurnsRemaining = festivalTurnsRemaining;
	}
}

GameStateView::GameStateView()
	: currentTurn(0), maxTurn(0), currentPlayerName(""), activePlayerIndex(0), hasRolledDice(false),
	  hasUsedSkillCard(false), extraRollAvailable(false) {}

GameStateView::GameStateView(const GameState& state) : GameStateView() {
	refresh(state);
}

void GameStateView::refresh(
	const GameState& state,
	const Board* board,
	const std::vector<FestivalEffectSnapshot>* festivalEffects) {
	currentTurn = state.getCurrentTurn();
	maxTurn = state.getMaxTurn();
	activePlayerIndex = state.getActivePlayerIndex();
	hasRolledDice = state.getHasRolledDice();
	hasUsedSkillCard = state.getHasUsedSkillCard();
	extraRollAvailable = state.getExtraRollAvailable();

	currentPlayerName.clear();
	if (const Player* activePlayer = state.getActivePlayer()) {
		currentPlayerName = activePlayer->getUsername();
	}

	players.clear();
	for (const Player* player : state.getPlayers()) {
		if (player == nullptr) {
			continue;
		}
		PlayerView view;
		view.username = player->getUsername();
		view.money = player->getMoney();
		view.position = player->getPosition();
		view.status = player->getStatus();
		view.propertyCount = static_cast<int>(player->getProperties().size());
		view.skillCardCount = static_cast<int>(player->getSkillCards().size());
		view.colorIndex = player->getColorIndex();
		players.push_back(view);
	}

	std::map<std::string, int> ownerColorIndexByName;
	for (std::size_t i = 0; i < players.size(); ++i) {
		ownerColorIndexByName[players[i].username] = players[i].colorIndex;
	}

	properties.clear();

	tiles.clear();
	if (board) {
		std::map<std::string, std::pair<int, int>> festivalByCode;
		if (festivalEffects) {
			for (const FestivalEffectSnapshot& snapshot : *festivalEffects) {
				PropertyTile* property = snapshot.getProperty();
				if (!property || snapshot.getTurnsRemaining() <= 0) {
					continue;
				}
				festivalByCode[property->getCode()] = {
					snapshot.getMultiplier(),
					snapshot.getTurnsRemaining()
				};
			}
		}

		for (int i = 0; i < board->getSize(); ++i) {
			Tile* tile = board->getTile(i);
			if (!tile) continue;
			TileData tv;
			tv.index = i;
			tv.code = tile->getCode();
			tv.name = tile->getName();
			tv.type = tile->getType();
			tv.color = Color::DEFAULT;
			tv.subtitle = tileSubtitle(*tile);

			if (auto* property = dynamic_cast<PropertyTile*>(tile)) {
				int festivalMultiplier = 1;
				int festivalTurnsRemaining = 0;
				const auto festivalIt = festivalByCode.find(property->getCode());
				if (festivalIt != festivalByCode.end()) {
					festivalMultiplier = festivalIt->second.first;
					festivalTurnsRemaining = festivalIt->second.second;
				}

				tv.isOwnable = true;
				tv.price = property->getPrice().getAmount();
				tv.mortgageValue = property->getMortgageValue().getAmount();
				tv.propertyStatus = property->getStatus();
				tv.ownerName = property->getOwner() ? property->getOwner()->getUsername() : "";
				const auto ownerIt = ownerColorIndexByName.find(tv.ownerName);
				tv.ownerColorIndex = ownerIt != ownerColorIndexByName.end() ? ownerIt->second : -1;
				tv.isMortgaged = property->isMortgaged();
				tv.buildingLevel = property->getBuildingLevel();
				tv.festivalMultiplier = festivalMultiplier;
				tv.festivalTurnsRemaining = festivalTurnsRemaining;

				PropertyView propertySnapshot;
				populatePropertyView(propertySnapshot, *property, festivalMultiplier, festivalTurnsRemaining);
				properties.push_back(propertySnapshot);
			}

			if (auto* street = dynamic_cast<StreetTile*>(tile)) {
				tv.color = street->getColor();
				tv.hasHotel = street->hasHotel();
				tv.houseCount = street->hasHotel() ? 4 : street->getBuildingLevel();
				tv.rentLevels = street->getRentLevels();
				tv.houseCost = street->getHouseCost().getAmount();
				tv.hotelCost = street->getHotelCost().getAmount();
			} else if (auto* railroad = dynamic_cast<RailroadTile*>(tile)) {
				tv.color = Color::GRAY;
				tv.rentLevels = railroad->getRentTable();
			} else if (dynamic_cast<UtilityTile*>(tile)) {
				tv.color = Color::LIGHT_BLUE;
			}
			tiles.push_back(tv);
		}
	}
}

int GameStateView::getCurrentTurn() const {
	return currentTurn;
}

int GameStateView::getMaxTurn() const {
	return maxTurn;
}

const std::string& GameStateView::getActivePlayerName() const {
	return currentPlayerName;
}

int GameStateView::getActivePlayerIndex() const {
	return activePlayerIndex;
}

const std::vector<PlayerView>& GameStateView::getPlayers() const {
	return players;
}

const std::vector<PropertyView>& GameStateView::getProperties() const {
	return properties;
}

bool GameStateView::getHasRolledDice() const {
	return hasRolledDice;
}

bool GameStateView::getHasUsedSkillCard() const {
	return hasUsedSkillCard;
}

bool GameStateView::getExtraRollAvailable() const {
	return extraRollAvailable;
}

const PlayerView* GameStateView::getActivePlayer() const {
	if (!currentPlayerName.empty()) {
		for (const PlayerView& player : players) {
			if (player.username == currentPlayerName) {
				return &player;
			}
		}
	}
	if (activePlayerIndex >= 0 && activePlayerIndex < static_cast<int>(players.size())) {
		return &players[static_cast<size_t>(activePlayerIndex)];
	}
	return nullptr;
}

std::string GameStateView::render(const GameState& state) const {
	GameStateView snapshot(state);
	std::string phase = "SETUP";
	if (state.getPhase() == GamePhase::RUNNING) {
		phase = "RUNNING";
	} else if (state.getPhase() == GamePhase::FINISHED) {
		phase = "FINISHED";
	}

	std::string result = "Phase: " + phase + " | Turn: " + std::to_string(snapshot.getCurrentTurn());
	if (!snapshot.getActivePlayerName().empty()) {
		result += " | Active: " + snapshot.getActivePlayerName();
	}
	return result;
}
