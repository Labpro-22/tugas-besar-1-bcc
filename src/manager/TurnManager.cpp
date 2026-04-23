#include "manager/header/TurnManager.hpp"

TurnManager::TurnManager(int playerCount) : playerCount(playerCount), currentPlayerIndex(0), turnNumber(1) {}

int TurnManager::getCurrentPlayerIndex() const {
	return currentPlayerIndex;
}

int TurnManager::getTurnNumber() const {
	return turnNumber;
}

int TurnManager::getPlayerCount() const {
	return playerCount;
}

void TurnManager::setPlayerCount(int count) {
	playerCount = count < 0 ? 0 : count;
}

void TurnManager::setTurnNumber(int value) {
	turnNumber = value < 1 ? 1 : value;
}

void TurnManager::setCurrentPlayerIndex(int value) {
	if (playerCount <= 0) {
		currentPlayerIndex = 0;
		return;
	}
	currentPlayerIndex = value % playerCount;
	if (currentPlayerIndex < 0) {
		currentPlayerIndex += playerCount;
	}
}

void TurnManager::nextTurn() {
	if (playerCount <= 0) {
		return;
	}
	currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
	++turnNumber;
}

void TurnManager::removePlayer(int playerIndex) {
	if (playerCount <= 0 || playerIndex < 0 || playerIndex >= playerCount) {
		return;
	}
	--playerCount;
	if (playerIndex < currentPlayerIndex) {
		--currentPlayerIndex;
	} else if (playerIndex == currentPlayerIndex) {
		if (currentPlayerIndex >= playerCount) {
			currentPlayerIndex = 0;
		}
	}
	if (playerCount <= 0) {
		currentPlayerIndex = 0;
	} else {
		currentPlayerIndex %= playerCount;
	}
}

int TurnManager::getActivePlayerCount() const {
	return playerCount;
}

bool TurnManager::isMaxTurnReached(int maxTurn) const {
	return maxTurn > 0 && turnNumber > maxTurn;
}

int TurnManager::getNextPlayerIndexAfter(int index) const {
	if (playerCount <= 0) {
		return 0;
	}
	const int normalized = ((index % playerCount) + playerCount) % playerCount;
	return (normalized + 1) % playerCount;
}
