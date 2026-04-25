#include "core/state/header/GameState.hpp"

#include <algorithm>

#include "core/state/header/GameStateView.hpp"

GameState::GameState()
	: phase(GamePhase::SETUP), currentTurn(0), maxTurn(0), activePlayerIndex(0), hasRolledDice(false),
	  hasUsedSkillCard(false), extraRollAvailable(false), consecutiveDoubles(0) {}

GamePhase GameState::getPhase() const {
	return phase;
}

int GameState::getTurn() const {
	return currentTurn;
}

int GameState::getCurrentTurn() const {
	return currentTurn;
}

int GameState::getMaxTurn() const {
	return maxTurn;
}

const std::vector<Player*>& GameState::getPlayers() const {
	return players;
}

Player* GameState::getActivePlayer() const {
	if (!turnOrder.empty()) {
		if (activePlayerIndex >= 0 && activePlayerIndex < static_cast<int>(turnOrder.size())) {
			return turnOrder[activePlayerIndex];
		}
		return nullptr;
	}

	if (activePlayerIndex >= 0 && activePlayerIndex < static_cast<int>(players.size())) {
		return players[activePlayerIndex];
	}
	return nullptr;
}

int GameState::getActivePlayerIndex() const {
	return activePlayerIndex;
}

const std::vector<Player*>& GameState::getTurnOrder() const {
	return turnOrder;
}

bool GameState::getHasRolledDice() const {
	return hasRolledDice;
}

bool GameState::getHasUsedSkillCard() const {
	return hasUsedSkillCard;
}

bool GameState::getExtraRollAvailable() const {
	return extraRollAvailable;
}

int GameState::getConsecutiveDoubles() const {
	return consecutiveDoubles;
}

void GameState::setPhase(GamePhase value) {
	phase = value;
}

void GameState::setCurrentTurn(int value) {
	currentTurn = std::max(0, value);
}

void GameState::setMaxTurn(int value) {
	maxTurn = std::max(0, value);
}

void GameState::setPlayers(const std::vector<Player*>& value) {
	players = value;
	if (turnOrder.empty()) {
		turnOrder = players;
	}
	setActivePlayerIndex(activePlayerIndex);
}

void GameState::setActivePlayerIndex(int value) {
	const int sourceSize = !turnOrder.empty() ? static_cast<int>(turnOrder.size()) : static_cast<int>(players.size());
	if (sourceSize <= 0) {
		activePlayerIndex = 0;
		return;
	}
	activePlayerIndex = std::max(0, std::min(value, sourceSize - 1));
}

void GameState::setTurnOrder(const std::vector<Player*>& value) {
	turnOrder = value;
	setActivePlayerIndex(activePlayerIndex);
}

void GameState::setHasRolledDice(bool value) {
	hasRolledDice = value;
}

void GameState::setHasUsedSkillCard(bool value) {
	hasUsedSkillCard = value;
}

void GameState::setExtraRollAvailable(bool value) {
	extraRollAvailable = value;
}

void GameState::incrementConsecutiveDoubles() {
	++consecutiveDoubles;
}

void GameState::resetConsecutiveDoubles() {
	consecutiveDoubles = 0;
}

void GameState::resetTurnFlags() {
	hasRolledDice = false;
	hasUsedSkillCard = false;
	extraRollAvailable = false;
	consecutiveDoubles = 0;
}

bool GameState::canSave() const {
	return phase == GamePhase::RUNNING &&
		!players.empty() &&
		!hasRolledDice &&
		!hasUsedSkillCard &&
		!extraRollAvailable;
}

GameStateView GameState::toView() const {
	return GameStateView(*this);
}

void GameState::nextTurn() {
	++currentTurn;
}
