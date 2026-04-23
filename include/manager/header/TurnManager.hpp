#ifndef MANAGER_HEADER_TURN_MANAGER_HPP
#define MANAGER_HEADER_TURN_MANAGER_HPP

class TurnManager {
public:
	explicit TurnManager(int playerCount = 0);

	int getCurrentPlayerIndex() const;
	int getTurnNumber() const;
	int getPlayerCount() const;

	void setPlayerCount(int count);
	void setTurnNumber(int value);
	void setCurrentPlayerIndex(int value);
	void nextTurn();
	void removePlayer(int playerIndex);
	int getActivePlayerCount() const;
	bool isMaxTurnReached(int maxTurn) const;
	int getNextPlayerIndexAfter(int index) const;

private:
	int playerCount;
	int currentPlayerIndex;
	int turnNumber;

protected:
};

#endif
