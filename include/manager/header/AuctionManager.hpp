#ifndef MANAGER_HEADER_AUCTION_MANAGER_HPP
#define MANAGER_HEADER_AUCTION_MANAGER_HPP

#include <vector>
#include "models/Money.hpp"

class PropertyTile;
class Player;
class Bank;
class TransactionLogger;
class Board;

class AuctionManager {
public:
	AuctionManager();

	bool isActive() const;
	int getHighestBid() const;

	void startAuction(int openingBid = 0);
	bool placeBid(int bid);
	void closeAuction();

	bool runAuction(PropertyTile& property, Player& trigger,
		const std::vector<Player*>& players, Bank& bank, Board& board,
		TransactionLogger& logger, int currentTurn);

private:
	bool active;
	int highestBid;

protected:
};

#endif
