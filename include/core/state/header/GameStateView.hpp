#pragma once

#include <string>
#include <vector>

#include "core/state/header/GameState.hpp"
#include "utils/Types.hpp"

class Board;
class FestivalEffectSnapshot;

// Read-only snapshot passed to IGameView::showBoard() and other render calls.
// Task 5 (GameEngine) populates this; UI layer only reads it.
class GameStateView {
public:
    vector<TileData>    tiles;           // all board tiles in index order (0-based)
    vector<PlayerView>  players;         // all players (including bankrupt)
    vector<PropertyView> properties;     // all ownable properties

    int    currentTurn{0};
    int    maxTurn{0};
    string currentPlayerName;
	int    activePlayerIndex{0};
	bool   hasRolledDice{false};
	bool   hasUsedSkillCard{false};
	bool   extraRollAvailable{false};

	GameStateView();
	explicit GameStateView(const GameState& state);

	void refresh(const GameState& state,
		const Board* board = nullptr,
		const std::vector<FestivalEffectSnapshot>* festivalEffects = nullptr);

	int getCurrentTurn() const;
	int getMaxTurn() const;
	const std::string& getActivePlayerName() const;
	int getActivePlayerIndex() const;
	const std::vector<PlayerView>& getPlayers() const;
	const std::vector<PropertyView>& getProperties() const;
	bool getHasRolledDice() const;
	bool getHasUsedSkillCard() const;
	bool getExtraRollAvailable() const;
	const PlayerView* getActivePlayer() const;

	std::string render(const GameState& state) const;
};
