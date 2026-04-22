#pragma once

#include <string>
#include <vector>

#include "utils/Structs.hpp"

// Read-only snapshot passed to IGameView::showBoard() and other render calls.
// Task 5 (GameEngine) populates this; UI layer only reads it.
class GameStateView {
public:
    vector<TileView>    tiles;           // all board tiles in index order (0-based)
    vector<PlayerView>  players;         // all players (including bankrupt)
    vector<PropertyView> properties;     // all ownable properties

    int    currentTurn{0};
    int    maxTurn{0};
    string currentPlayerName;
};
