#ifndef CORE_ENGINE_HEADER_GAME_ENGINE_HPP
#define CORE_ENGINE_HEADER_GAME_ENGINE_HPP

#include <string>
#include <vector>

#include "core/command/header/CommandRegistry.hpp"
#include "core/Bank.hpp"
#include "core/FestivalManager.hpp"
#include "core/IGameRepository.hpp"
#include "core/config/header/GameConfig.hpp"
#include "core/log/header/TransactionLogger.hpp"
#include "core/state/header/GameState.hpp"
#include "manager/header/AuctionManager.hpp"
#include "manager/header/BankruptcyManager.hpp"
#include "manager/header/EventBus.hpp"
#include "manager/header/TurnManager.hpp"
#include "models/board/header/Board.hpp"
#include "models/board/header/Dice.hpp"
#include "models/cards/CardSystem.hpp"
#include "models/Money.hpp"

class Player;
class PropertyTile;
class StreetTile;

class GameEngine {
public:
	explicit GameEngine(GameState initialState = GameState());
	GameEngine(const GameEngine& other) = delete;
	GameEngine& operator=(const GameEngine& other) = delete;
	GameEngine(GameEngine&& other) noexcept = default;
	GameEngine& operator=(GameEngine&& other) noexcept = default;

	void initialize(const std::vector<Player*>& players, int maxTurn);
	bool loadConfiguration(const std::string& directory = "config");
	void start();
	void stop();
	void runGameLoop();

	bool isRunning() const;
	GameState& getState();
	const GameState& getState() const;

	CommandRegistry& getCommandRegistry();
	const CommandRegistry& getCommandRegistry() const;
	TurnManager& getTurnManager();
	AuctionManager& getAuctionManager();
	BankruptcyManager& getBankruptcyManager();
	EventBus& getEventBus();
	TransactionLogger& getTransactionLogger();
	Bank& getBank();
	Board& getBoard();
	Dice& getDice();
	GameConfig& getConfig();
	CardSystem& getCardSystem();
	bool processCommand(const std::string& input, Player& player);

	void setRepository(IGameRepository* r);
	bool saveGame(const std::string& id);
	bool loadGame(const std::string& id);
	void printWinners() const;

private:
	void buildDefaultBoard(const std::string& configDirectory);
	void executeTurn(Player& player);
	void prepareTurn(Player& player);
	void endTurn(Player& player);
	void rollDice(Player& player);
	void movePlayer(Player& player, int steps, bool awardGoSalary);
	void sendToJail(Player& player);
	void resolveLanding(Player& player, int diceTotal);
	void resolveCardLanding(Player& player, bool chance);
	void resolvePropertyLanding(Player& player, PropertyTile& property, int diceTotal);
	void resolveTaxLanding(Player& player, bool pph);
	void resolveFestivalLanding(Player& player);
	void useSkillCard(Player& player, int cardIndex);
	void mortgageProperty(Player& player, const std::string& code);
	void redeemProperty(Player& player, const std::string& code);
	void buildOnProperty(Player& player, const std::string& code);
	void printBoard() const;
	void printProperties(const Player& player) const;
	void printAkta(const std::string& code) const;
	void printLog(int lastN) const;
	void awardGoSalary(Player& player);
	bool auctionProperty(PropertyTile& property, Player& trigger);
	bool handleAuctionCommand(const std::string& input, Player& player);
	void checkBankruptcy(Player& player);
	std::string normalizeCommandToken(const std::string& input) const;
	void handleJailTurn(Player& player);
	void payJailFine(Player& player);
	void executeBankruptcy(Player& debtor, Player* creditor, Money obligation);
	void syncTurnManagerAfterLoad();

	GameState gameState;
	bool running;
	bool turnPrepared;
	int lastDiceTotal;
	CommandRegistry commandRegistry;
	TurnManager turnManager;
	AuctionManager auctionManager;
	BankruptcyManager bankruptcyManager;
	EventBus eventBus;
	TransactionLogger transactionLogger;
	Bank bank;
	FestivalManager festivalManager;
	GameConfig gameConfig;
	Dice dice;
	Board board;
	CardSystem cardSystem;
	std::string configDirectory;
	IGameRepository* repository;

protected:
};

#endif
