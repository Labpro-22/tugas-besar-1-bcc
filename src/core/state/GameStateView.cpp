#include "core/state/header/GameStateView.hpp"

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
			case TileType::STREET: return "Property";
			case TileType::RAILROAD: return "Station";
			case TileType::UTILITY: return "Utility";
			case TileType::CHANCE: return "Chance";
			case TileType::COMMUNITY_CHEST: return "Community Chest";
			case TileType::FESTIVAL: return "Festival";
			case TileType::TAX_PPH: return "Income Tax";
			case TileType::TAX_PBM: return "Luxury Tax";
			case TileType::GO: return "Collect Salary";
			case TileType::JAIL: return "Just Visiting";
			case TileType::FREE_PARKING: return "Take a Break";
			case TileType::GO_TO_JAIL: return "Move to Jail";
		}
		return "";
	}

	void populatePropertyView(PropertyView& snapshot, const PropertyTile& property) {
		snapshot.code = property.getCode();
		snapshot.name = property.getName();
		snapshot.ownerName = property.getOwner() ? property.getOwner()->getUsername() : "";
		snapshot.status = property.getStatus();
		snapshot.buildingLevel = property.getBuildingLevel();
	}
}

GameStateView::GameStateView()
	: currentTurn(0), maxTurn(0), currentPlayerName(""), activePlayerIndex(0), hasRolledDice(false),
	  hasUsedSkillCard(false) {}

GameStateView::GameStateView(const GameState& state) : GameStateView() {
	refresh(state);
}

void GameStateView::refresh(const GameState& state, const Board* board) {
	currentTurn = state.getCurrentTurn();
	maxTurn = state.getMaxTurn();
	activePlayerIndex = state.getActivePlayerIndex();
	hasRolledDice = state.getHasRolledDice();
	hasUsedSkillCard = state.getHasUsedSkillCard();

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
		players.push_back(view);
	}

	properties.clear();

	tiles.clear();
	if (board) {
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
				tv.isOwnable = true;
				tv.price = property->getPrice().getAmount();
				tv.mortgageValue = property->getMortgageValue().getAmount();
				tv.propertyStatus = property->getStatus();
				tv.ownerName = property->getOwner() ? property->getOwner()->getUsername() : "";
				tv.isMortgaged = property->isMortgaged();
				tv.buildingLevel = property->getBuildingLevel();

				PropertyView propertySnapshot;
				populatePropertyView(propertySnapshot, *property);
				properties.push_back(propertySnapshot);
			}

			if (auto* street = dynamic_cast<StreetTile*>(tile)) {
				tv.color = street->getColor();
				tv.hasHotel = street->hasHotel();
				tv.houseCount = street->hasHotel() ? 4 : street->getBuildingLevel();
			} else if (dynamic_cast<RailroadTile*>(tile)) {
				tv.color = Color::GRAY;
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
