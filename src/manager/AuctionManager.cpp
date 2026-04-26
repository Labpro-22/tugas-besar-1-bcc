#include "manager/header/AuctionManager.hpp"

#include <algorithm>
#include <iostream>

#include "controllers/PlayerController.hpp"
#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "tile/header/PropertyTile.hpp"
#include "core/Bank.hpp"
#include "core/log/header/TransactionLogger.hpp"

AuctionManager::AuctionManager() : active(false), highestBid(0) {}

bool AuctionManager::isActive() const {
	return active;
}

int AuctionManager::getHighestBid() const {
	return highestBid;
}

void AuctionManager::startAuction(int openingBid) {
	active = true;
	highestBid = openingBid;
}

bool AuctionManager::placeBid(int bid) {
	if (!active || bid <= highestBid) {
		return false;
	}
	highestBid = bid;
	return true;
}

void AuctionManager::closeAuction() {
	active = false;
}

bool AuctionManager::runAuction(PropertyTile& property, Player& trigger,
	const std::vector<Player*>& players, Bank& bank, Board& board,
	TransactionLogger& logger, int currentTurn) {
	std::cout << "Lelang " << property.getName() << " dimulai.\n";
	Player* winner = nullptr;
	int highestBid = -1;
	int passCount = 0;
	const int total = static_cast<int>(players.size());
	int activeParticipants = 0;
	for (Player* bidder : players) {
		if (bidder && !bidder->isBankrupt()) {
			++activeParticipants;
		}
	}
	if (activeParticipants <= 0) {
		return false;
	}

	int index = 0;
	for (int i = 0; i < total; ++i) {
		if (players[i] == &trigger) {
			index = (i + 1) % total;
			break;
		}
	}

	const int passThreshold = std::max(1, activeParticipants - 1);
	while (true) {
		Player* bidder = players[index];
		index = (index + 1) % total;
		if (!bidder || bidder->isBankrupt()) {
			continue;
		}
		AuctionDecision decision;
		if (PlayerController* controller = bidder->getController()) {
			decision = controller->decideAuction(highestBid, bidder->getMoney());
		}
		if (decision.action == AuctionAction::BID) {
			int bid = decision.bidAmount;
			if (bid <= highestBid) {
				bid = std::max(0, highestBid + 1);
			}
			if (bid > highestBid && bidder->canAfford(Money(bid))) {
				highestBid = bid;
				winner = bidder;
				passCount = 0;
				logger.log(currentTurn, bidder->getUsername(), "LELANG_BID",
					"Bid " + Money(bid).toString() + " untuk " + property.getLabel());
				continue;
			}
		}

		if (!winner && passCount + 1 >= activeParticipants) {
			const int forcedBid = std::max(0, highestBid + 1);
			if (bidder->canAfford(Money(forcedBid))) {
				highestBid = forcedBid;
				winner = bidder;
				passCount = 0;
				logger.log(currentTurn, bidder->getUsername(), "LELANG_BID",
					"Bid paksa " + Money(forcedBid).toString() + " untuk " + property.getLabel());
				continue;
			}
		}

		logger.log(currentTurn, bidder->getUsername(), "LELANG_PASS",
			"Pass pada lelang " + property.getLabel());
		++passCount;
		if (!winner && passCount >= activeParticipants) {
			break;
		}
		if (winner && passCount >= passThreshold) {
			break;
		}
	}

	if (!winner) {
		logger.log(currentTurn, "SYSTEM", "LELANG",
			"Lelang tanpa pemenang untuk " + property.getLabel());
		std::cout << "Lelang selesai tanpa pemenang.\n";
		return false;
	}

	bank.collectFromPlayer(*winner, Money(highestBid), "Lelang");
	property.setOwner(winner);
	property.setStatus(PropertyStatus::OWNED);
	winner->addProperty(&property);
	board.updateMonopolies();
	logger.log(currentTurn, winner->getUsername(), "LELANG",
		"Menang lelang " + property.getLabel() + " " + Money(highestBid).toString());
	std::cout << "Pemenang lelang: " << winner->getUsername() << "\n";
	return true;
}
