#include "core/engine/header/GameEngine.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "controllers/PlayerController.hpp"
#include "core/state/header/GameStateView.hpp"
#include "models/Money.hpp"
#include "models/Player.hpp"
#include "models/board/header/ColorGroup.hpp"
#include "models/event/header/GameEvent.hpp"
#include "tile/header/ChanceTile.hpp"
#include "tile/header/CommunityChestTile.hpp"
#include "tile/header/FestivalTile.hpp"
#include "tile/header/FreeParkingTile.hpp"
#include "tile/header/GoTile.hpp"
#include "tile/header/GoToJailTile.hpp"
#include "tile/header/JailTile.hpp"
#include "tile/header/PBMTile.hpp"
#include "tile/header/PPHTile.hpp"
#include "tile/header/PropertyTile.hpp"
#include "tile/header/RailroadTile.hpp"
#include "tile/header/StreetTile.hpp"
#include "tile/header/Tile.hpp"
#include "tile/header/UtilityTile.hpp"
#include "utils/Exceptions.hpp"
#include "utils/GameUtils.hpp"

namespace {
	struct PropertyRow {
		int id{0};
		std::string code;
		std::string name;
		std::string kind;
		std::string colorText;
		int price{0};
		int mortgage{0};
		int houseCost{0};
		int hotelCost{0};
		std::vector<int> rents;
	};

	std::vector<int> completeRentLevels(const std::vector<int>& parsed) {
		if (parsed.empty()) {
			return {0, 0, 0, 0, 0, 0};
		}
		if (parsed.size() >= 6) {
			return std::vector<int>(parsed.begin(), parsed.begin() + 6);
		}

		std::vector<int> rents(6, parsed.front());
		if (parsed.size() == 1) {
			for (int i = 1; i < 6; ++i) {
				rents[i] = parsed.front() * (i + 1);
			}
			return rents;
		}

		for (std::size_t i = 0; i < parsed.size() && i < rents.size(); ++i) {
			rents[i] = parsed[i];
		}

		const int startIndex = static_cast<int>(parsed.size()) - 1;
		const int endValue = parsed.back();
		for (int i = startIndex + 1; i < 6; ++i) {
			rents[i] = endValue;
		}

		if (parsed.size() == 2) {
			const int first = parsed.front();
			const int last = parsed.back();
			for (int i = 0; i < 6; ++i) {
				rents[i] = first + (last - first) * i / 5;
			}
		}

		if (parsed.size() == 3) {
			const int first = parsed[0];
			const int second = parsed[1];
			const int last = parsed[2];
			rents[0] = first;
			rents[1] = second;
			for (int i = 2; i < 6; ++i) {
				rents[i] = second + (last - second) * (i - 1) / 4;
			}
		}

		return rents;
	}

	bool parsePropertyRow(const std::string& line, PropertyRow& row) {
		const std::vector<std::string> tokens = splitTokens(line);
		if (tokens.empty()) {
			return false;
		}

		if (!parseIntToken(tokens[0], row.id) || tokens.size() < 7) {
			return false;
		}

		row.code = tokens[1];
		row.name = tokens[2];
		row.kind = tokens[3];
		row.colorText = tokens[4];
		if (!parseIntToken(tokens[5], row.price) ||
			!parseIntToken(tokens[6], row.mortgage)) {
			return false;
		}

		const std::string normalizedKind = lower(row.kind);
		if (normalizedKind == "street") {
			if (tokens.size() < 9 ||
				!parseIntToken(tokens[7], row.houseCost) ||
				!parseIntToken(tokens[8], row.hotelCost)) {
				return false;
			}

			for (std::size_t i = 9; i < tokens.size(); ++i) {
				int rent = 0;
				if (parseIntToken(tokens[i], rent)) {
					row.rents.push_back(rent);
				}
			}
		}

		return true;
	}

	std::vector<PlayerSummary> buildPlayerSummaries(const std::vector<Player*>& players) {
		std::vector<PlayerSummary> summaries;
		for (const Player* player : players) {
			if (!player || player->isBankrupt()) {
				continue;
			}

			PlayerSummary summary;
			summary.username = player->getUsername();
			summary.money = player->getMoney();
			summary.propertyCount = static_cast<int>(player->getProperties().size());
			summary.cardCount = static_cast<int>(player->getSkillCards().size());
			summary.isBankrupt = false;
			summaries.push_back(summary);
		}

		std::sort(summaries.begin(), summaries.end(), [](const PlayerSummary& a, const PlayerSummary& b) {
			if (a.money != b.money) {
				return a.money > b.money;
			}
			if (a.propertyCount != b.propertyCount) {
				return a.propertyCount > b.propertyCount;
			}
			if (a.cardCount != b.cardCount) {
				return a.cardCount > b.cardCount;
			}
			return a.username < b.username;
		});
		return summaries;
	}

	WinnerInfo buildWinnerInfoFromStanding(const std::vector<PlayerSummary>& standing, const std::string& winCondition) {
		WinnerInfo info;
		info.players = standing;
		info.winCondition = winCondition;
		if (!standing.empty()) {
			const PlayerSummary& top = standing.front();
			for (const PlayerSummary& s : standing) {
				if (s.money == top.money && s.propertyCount == top.propertyCount && s.cardCount == top.cardCount) {
					info.winners.push_back(s.username);
				} else {
					break;
				}
			}
		}
		return info;
	}

}

GameEngine::GameEngine(GameState initialState)
	: gameState(initialState), running(false), turnPrepared(false), lastDiceTotal(0), commandRegistry(),
	  turnManager(), auctionManager(), bankruptcyManager(), eventBus(), transactionLogger(), bank(),
	  festivalManager(), gameConfig(), dice(), board(), cardSystem(), configDirectory("config"),
	  numTiles(40), repository(nullptr) {
	bankruptcyManager.setBankruptcyThreshold(0);
	eventBus.subscribe(&transactionLogger);
	loadConfiguration(configDirectory, numTiles);
}

void GameEngine::initialize(const std::vector<Player*>& players, int maxTurn) {
	transactionLogger.clear();
	gameState.setPlayers(players);
	gameState.setTurnOrder(players);
	gameState.setActivePlayerIndex(0);
	gameState.setCurrentTurn(1);
	gameState.setMaxTurn(maxTurn > 0 ? maxTurn : gameConfig.getMaxTurns());
	turnPrepared = false;

	turnManager.setPlayerCount(static_cast<int>(players.size()));
	turnManager.setCurrentPlayerIndex(0);
	turnManager.setTurnNumber(1);
	gameState.setCurrentTurn(turnManager.getTurnNumber());
}

bool GameEngine::loadConfiguration(const std::string& directory, int targetNumTiles) {
	configDirectory = directory.empty() ? "config" : directory;
	numTiles = targetNumTiles > 0 ? targetNumTiles : 40;
	const std::string sizeDir = configDirectory + "/" + std::to_string(numTiles);

	bool loaded = gameConfig.loadFromDirectory(configDirectory.c_str());
	if (fileExists(sizeDir + "/misc.txt") ||
		fileExists(sizeDir + "/tax.txt") ||
		fileExists(sizeDir + "/special.txt")) {
		loaded = gameConfig.loadFromDirectory(sizeDir.c_str()) || loaded;
	}
	buildDefaultBoard(configDirectory, numTiles);
	return loaded;
}

void GameEngine::buildDefaultBoard(const std::string& directory, int targetNumTiles) {
	board.clear();
	const std::string sizeDir = directory + "/" + std::to_string(targetNumTiles);

	// Parse property.txt
	std::map<std::string, PropertyRow> propertyMap;
	{
		std::ifstream file(sizeDir + "/property.txt");
		std::string line;
		while (std::getline(file, line)) {
			PropertyRow row;
			if (!parsePropertyRow(line, row)) {
				continue;
			}
			propertyMap[normalizeCode(row.code)] = row;
		}
	}

	// Parse aksi.txt
	struct AksiRow {
		int id{0};
		std::string code;
		std::string name;
		std::string kind;
	};
	std::map<std::string, AksiRow> aksiMap;
	{
		std::ifstream file(sizeDir + "/aksi.txt");
		std::string line;
		while (std::getline(file, line)) {
			const std::vector<std::string> tokens = splitTokens(line);
			if (tokens.size() < 4) {
				continue;
			}
			AksiRow row;
			if (!parseIntToken(tokens[0], row.id)) {
				continue;
			}
			row.code = tokens[1];
			row.name = tokens[2];
			row.kind = tokens[3];
			aksiMap[normalizeCode(row.code)] = row;
		}
	}

	// Parse board.txt and create tiles
	std::ifstream boardFile(sizeDir + "/board.txt");
	std::string line;
	int index = 0;
	while (std::getline(boardFile, line) && index < targetNumTiles) {
		const std::vector<std::string> tokens = splitTokens(line);
		if (tokens.empty()) {
			continue;
		}

		const std::string code = normalizeCode(tokens[0]);

		// Try property first
		auto propIt = propertyMap.find(code);
		if (propIt != propertyMap.end()) {
			const PropertyRow& row = propIt->second;
			const std::string normalizedKind = lower(row.kind);
			if (normalizedKind == "street") {
				auto street = std::make_unique<StreetTile>(
					index, code, displayName(row.name),
					Money(row.price), Money(row.mortgage),
					parseColor(row.colorText),
					Money(row.houseCost), Money(row.hotelCost),
					completeRentLevels(row.rents)
				);
				ColorGroup* group = board.ensureColorGroup(street->getColor());
				group->addStreet(street.get());
				board.addTile(std::move(street));
				} else if (normalizedKind == "railroad") {
					board.addTile(std::make_unique<RailroadTile>(
						index, code, displayName(row.name), Money(row.price), Money(row.mortgage),
						gameConfig.getRailroadRentTable()
					));
				} else if (normalizedKind == "utility") {
					board.addTile(std::make_unique<UtilityTile>(
						index, code, displayName(row.name), Money(row.price), Money(row.mortgage),
						gameConfig.getUtilityMultiplierTable()
					));
				}
			++index;
			continue;
		}

		// Try aksi
		auto aksiIt = aksiMap.find(code);
		if (aksiIt != aksiMap.end()) {
			const AksiRow& row = aksiIt->second;
			const std::string kind = lower(row.kind);
			if (kind == "go" || kind == "spesial") {
				board.addTile(std::make_unique<GoTile>(index, code, displayName(row.name), gameConfig.getGoSalary()));
			} else if (kind == "jail") {
				board.addTile(std::make_unique<JailTile>(index, code, displayName(row.name), gameConfig.getJailFine()));
			} else if (kind == "free_parking") {
				board.addTile(std::make_unique<FreeParkingTile>(index, code, displayName(row.name)));
			} else if (kind == "go_to_jail") {
				board.addTile(std::make_unique<GoToJailTile>(index, code, displayName(row.name)));
			} else if (kind == "chance" || kind == "kartu") {
				// Check if it's chance or community based on code
				if (code == "KSP") {
					board.addTile(std::make_unique<ChanceTile>(index, code, displayName(row.name)));
				} else if (code == "DNU") {
					board.addTile(std::make_unique<CommunityChestTile>(index, code, displayName(row.name)));
				} else {
					board.addTile(std::make_unique<ChanceTile>(index, code, displayName(row.name)));
				}
			} else if (kind == "community") {
				board.addTile(std::make_unique<CommunityChestTile>(index, code, displayName(row.name)));
			} else if (kind == "festival") {
				board.addTile(std::make_unique<FestivalTile>(index, code, displayName(row.name)));
			} else if (kind == "tax_pph" || kind == "pajak") {
				if (code == "PPH") {
					board.addTile(std::make_unique<PPHTile>(index, code, displayName(row.name), gameConfig.getPphFlat(), gameConfig.getPphPercentage()));
				} else if (code == "PBM") {
					board.addTile(std::make_unique<PBMTile>(index, code, displayName(row.name), gameConfig.getPbmFlat()));
				} else {
					board.addTile(std::make_unique<PPHTile>(index, code, displayName(row.name), gameConfig.getPphFlat(), gameConfig.getPphPercentage()));
				}
			} else if (kind == "tax_pbm") {
				board.addTile(std::make_unique<PBMTile>(index, code, displayName(row.name), gameConfig.getPbmFlat()));
			} else {
				board.addTile(std::make_unique<FreeParkingTile>(index, code, displayName(row.name)));
			}
			++index;
			continue;
		}

		// Unknown tile
		throw InvalidBoardConfigException("Unknown tile code '" + code + "' at position " + std::to_string(index));
	}

	if (index != targetNumTiles) {
		throw InvalidBoardConfigException("Board layout has " + std::to_string(index) + " tiles, expected " + std::to_string(targetNumTiles));
	}
}

void GameEngine::start() {
	running = true;
	gameState.setPhase(GamePhase::RUNNING);
}

void GameEngine::stop() {
	running = false;
	gameState.setPhase(GamePhase::FINISHED);
	eventBus.publish(GameOverEvent(gameState.getCurrentTurn(), "SYSTEM", "Game loop stopped"));
}

void GameEngine::runGameLoop() {
	if (gameState.getPlayers().empty()) {
		stop();
		return;
	}

	if (!isRunning()) {
		start();
	}

	while (running) {
		if (isGameOver()) {
			stop();
			break;
		}

		Player* active = gameState.getActivePlayer();
		if (!active) {
			stop();
			break;
		}
		executeTurn(*active);
	}
}

bool GameEngine::isRunning() const {
	return running;
}

GameState& GameEngine::getState() {
	return gameState;
}

const GameState& GameEngine::getState() const {
	return gameState;
}

CommandRegistry& GameEngine::getCommandRegistry() {
	return commandRegistry;
}

const CommandRegistry& GameEngine::getCommandRegistry() const {
	return commandRegistry;
}

bool GameEngine::isGameOver() const {
	const std::vector<PlayerSummary> standing = buildPlayerSummaries(gameState.getPlayers());
	if (standing.size() <= 1) {
		return true;
	}
	if (gameState.getMaxTurn() > 0) {
		for (const Player* p : gameState.getPlayers()) {
			if (p && !p->isBankrupt() && p->getTurnCount() < gameState.getMaxTurn()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::string GameEngine::getGameOverReason() const {
	const std::vector<PlayerSummary> standing = buildPlayerSummaries(gameState.getPlayers());
	if (standing.size() <= 1) {
		return "Pemain terakhir yang tersisa";
	}
	if (gameState.getMaxTurn() > 0) {
		for (const Player* p : gameState.getPlayers()) {
			if (p && !p->isBankrupt() && p->getTurnCount() < gameState.getMaxTurn()) {
				return "";
			}
		}
		return "Batas giliran tercapai";
	}
	return "";
}

WinnerInfo GameEngine::getWinnerInfo(const std::string& winCondition) const {
	const std::vector<PlayerSummary> standing = buildPlayerSummaries(gameState.getPlayers());
	return buildWinnerInfoFromStanding(standing, winCondition);
}

TurnManager& GameEngine::getTurnManager() {
	return turnManager;
}

AuctionManager& GameEngine::getAuctionManager() {
	return auctionManager;
}

BankruptcyManager& GameEngine::getBankruptcyManager() {
	return bankruptcyManager;
}

EventBus& GameEngine::getEventBus() {
	return eventBus;
}

TransactionLogger& GameEngine::getTransactionLogger() {
	return transactionLogger;
}

Bank& GameEngine::getBank() {
	return bank;
}

FestivalManager& GameEngine::getFestivalManager() {
	return festivalManager;
}

Board& GameEngine::getBoard() {
	return board;
}

Dice& GameEngine::getDice() {
	return dice;
}

GameConfig& GameEngine::getConfig() {
	return gameConfig;
}

CardSystem& GameEngine::getCardSystem() {
	return cardSystem;
}

void GameEngine::ensureTurnPrepared(Player& player) {
	if (gameState.getPhase() != GamePhase::RUNNING) {
		start();
	}
	if (gameState.getActivePlayer() == &player && !turnPrepared) {
		prepareTurn(player);
	}
}

void GameEngine::setRepository(IGameRepository* r) {
	repository = r;
}

bool GameEngine::saveGame(const std::string& id) {
	if (!repository || !gameState.canSave() || dice.hasManualSet()) {
		return false;
	}
	const bool saved = repository->save(gameState, board, transactionLogger, festivalManager, cardSystem, id);
	if (saved) {
		transactionLogger.log(gameState.getCurrentTurn(), "SYSTEM", "SAVE", "Simpan permainan ke " + id);
	}
	return saved;
}

bool GameEngine::loadGame(const std::string& id) {
	if (!repository) {
		return false;
	}

	// Read board size from save file header for potential board rebuild
	std::ifstream check(id);
	if (check) {
		std::string header;
		std::getline(check, header);
		std::istringstream hs(header);
		int currentTurn = 0, maxTurn = 0, nPlayers = 0, savedBoardSize = 0;
		if (hs >> currentTurn >> maxTurn >> nPlayers >> savedBoardSize) {
			if (savedBoardSize > 0 && savedBoardSize != board.getSize()) {
				loadConfiguration(configDirectory, savedBoardSize);
			}
		}
	}

	if (!repository->loadInto(gameState, board, transactionLogger, festivalManager, cardSystem, id)) {
		return false;
	}
	syncTurnManagerAfterLoad();
	transactionLogger.log(gameState.getCurrentTurn(), "SYSTEM", "LOAD", "Muat permainan dari " + id);
	return true;
}

void GameEngine::syncTurnManagerAfterLoad() {
	turnManager.setPlayerCount(static_cast<int>(gameState.getTurnOrder().size()));
	turnManager.setTurnNumber(gameState.getCurrentTurn());
	turnManager.setCurrentPlayerIndex(gameState.getActivePlayerIndex());
	turnPrepared = false;
}

void GameEngine::printWinners() const {
	const WinnerInfo info = getWinnerInfo("Peringkat kekayaan");
	std::cout << "=== Peringkat kekayaan ===\n";
	int rank = 1;
	for (const PlayerSummary& summary : info.players) {
		std::cout << rank++ << ". " << summary.username
			<< " — uang " << summary.money.toString()
			<< ", properti " << summary.propertyCount
			<< ", kartu " << summary.cardCount << "\n";
	}
}

bool GameEngine::processCommand(const std::string& input, Player& player) {
	if (gameState.getPhase() != GamePhase::RUNNING) {
		start();
	}
	if (gameState.getActivePlayer() == &player && !turnPrepared) {
		prepareTurn(player);
	}

	std::istringstream stream(input);
	std::string first;
	stream >> first;
	if (first.empty()) {
		return false;
	}

	const std::string command = lower(first);
	try {
		if (command == "simpan") {
			std::string path;
			stream >> path;
			if (path.empty()) {
				path = "nimonspoli_save.txt";
			}
			if (!gameState.canSave() || dice.hasManualSet()) {
				std::cout << "Permainan belum bisa disimpan.\n";
				return false;
			}
			if (saveGame(path)) {
				std::cout << "Permainan disimpan ke " << path << ".\n";
			} else {
				std::cout << "Gagal menyimpan.\n";
			}
			return true;
		}

		if (command == "menang") {
			printWinners();
			return true;
		}

		if (command == "bangkrut") {
			bankruptcyManager.execute(player, nullptr, Money(0), gameState, board, bank, festivalManager, transactionLogger, turnManager, auctionManager, eventBus);
			return true;
		}

		if (command == "bayar_denda") {
			payJailFine(player);
			return true;
		}
		if (command == "bayar") {
			std::string second;
			if (stream >> second) {
				if (lower(second) == "denda") {
					payJailFine(player);
					return true;
				}
			}
			const bool executed = commandRegistry.parseAndExecute(input, *this, player);
			if (!executed) {
				transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "INVALID_COMMAND", input);
				std::cout << "Perintah tidak dikenal.\n";
				return false;
			}
			return true;
		}

		if (command == "lempar" || command == "lempar_dadu" || command == "roll_dice") {
			std::string maybeDadu;
			stream >> maybeDadu;
			if (gameState.getExtraRollAvailable()) {
				player.setHasUsedSkillCardThisTurn(false);
				gameState.setHasUsedSkillCard(false);
			}
			rollDice(player);
			return true;
		}

		if (command == "atur" || command == "atur_dadu") {
			int d1 = 0;
			int d2 = 0;
			if (command == "atur") {
				std::string dadu;
				stream >> dadu >> d1 >> d2;
			} else {
				stream >> d1 >> d2;
			}
			dice.setManual(d1, d2);
			transactionLogger.log(gameState.getCurrentTurn(),
				player.getUsername(),
				"ATUR_DADU",
				"Atur dadu: " + std::to_string(d1) + "+" + std::to_string(d2));
			if (gameState.getExtraRollAvailable()) {
				player.setHasUsedSkillCardThisTurn(false);
				gameState.setHasUsedSkillCard(false);
			}
			rollDice(player);
			return true;
		}

		if (command == "cetak" || command == "cetak_log" || command == "print") {
			std::string what;
			if (command == "cetak_log") {
				int n = 0;
				stream >> n;
				printLog(n);
				return true;
			}
			stream >> what;
			what = lower(what);
			if (what == "papan" || what == "board") {
				printBoard();
			} else if (what == "properti" || what == "properties") {
				printProperties(player);
			} else if (what == "akta") {
				std::string code;
				stream >> code;
				printAkta(code);
			} else if (what == "log") {
				int n = 0;
				stream >> n;
				printLog(n);
			}
			return true;
		}

		if (command == "gadai" || command == "mortgage") {
			std::string code;
			stream >> code;
			mortgageProperty(player, code);
			return true;
		}

		if (command == "tebus" || command == "redeem") {
			std::string code;
			stream >> code;
			redeemProperty(player, code);
			return true;
		}

		if (command == "bangun" || command == "build") {
			std::string code;
			stream >> code;
			buildOnProperty(player, code);
			return true;
		}

		if (command == "gunakan" || command == "gunakan_kemampuan" || command == "use_skill") {
			std::string maybeKemampuan;
			int index = 0;
			if (command == "gunakan" || command == "gunakan_kemampuan") {
				stream >> maybeKemampuan >> index;
			} else {
				stream >> index;
			}
			GameContext context{bank, festivalManager, transactionLogger, gameConfig, dice, const_cast<std::vector<Player*>&>(gameState.getPlayers()), board, gameState.getCurrentTurn(), &cardSystem, &auctionManager, &bankruptcyManager, &eventBus, &gameState, &turnManager};
			CardSystem::UseOutcome outcome = cardSystem.useSkillCard(player, index - 1, context, gameState.getHasRolledDice(), gameState.getExtraRollAvailable(), gameState.getHasUsedSkillCard());
			if (outcome.success && outcome.resolveLanding) {
				if (outcome.resolveLandingTarget) {
					resolveLanding(*outcome.resolveLandingTarget, lastDiceTotal);
				} else {
					resolveLanding(player, lastDiceTotal);
				}
			}
			return true;
		}

		if (command == "end_turn" || command == "selesai") {
			endTurn(player);
			return true;
		}

		if (handleAuctionCommand(input, player)) {
			return true;
		}

		const bool executed = commandRegistry.parseAndExecute(input, *this, player);
		if (!executed) {
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "INVALID_COMMAND", input);
			std::cout << "Perintah tidak dikenal.\n";
			return false;
		}
		return true;
	} catch (const std::exception& ex) {
		std::cout << "Error: " << ex.what() << "\n";
		transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "ERROR", ex.what());
		checkBankruptcy(player);
		return false;
	}
}

void GameEngine::executeTurn(Player& player) {
	eventBus.publish(TurnStartedEvent(gameState.getCurrentTurn(), player.getUsername(), "Turn started"));
	prepareTurn(player);

	PlayerController* controller = player.getController();
	if (controller == nullptr) {
		stop();
		return;
	}

	const std::string command = controller->chooseCommand(gameState.toView());
	if (!processCommand(command, player)) {
		std::cout << "Perintah gagal.\n";
	}
}

void GameEngine::prepareTurn(Player& player) {
	if (turnPrepared || player.isBankrupt()) {
		return;
	}

	turnPrepared = true;
	player.resetTurnFlags();
	gameState.resetTurnFlags();
	festivalManager.tickPlayerEffects(player);

	auto card = cardSystem.drawSkill();
	if (card) {
		std::cout << player.getUsername() << " mendapat kartu " << card->getName() << ".\n";
		if (static_cast<int>(player.getSkillCards().size()) < Player::MAX_SKILL_CARDS) {
			player.addSkillCard(card.release());
		} else {
			std::unique_ptr<SkillCard> overflow(std::move(card));
			std::vector<CardInfo> infos;
			for (SkillCard* owned : player.getSkillCards()) {
				infos.push_back(CardInfo{owned->getName(), owned->getDescription(), owned->getType()});
			}
			infos.push_back(CardInfo{overflow->getName(), overflow->getDescription(), overflow->getType()});

			int dropIndex = static_cast<int>(infos.size()) - 1;
			if (PlayerController* controller = player.getController()) {
				dropIndex = controller->decideDropCard(infos) - 1;
			}

			if (dropIndex >= 0 && dropIndex < static_cast<int>(player.getSkillCards().size())) {
				std::unique_ptr<SkillCard> dropped(player.removeSkillCard(dropIndex));
				cardSystem.discardSkill(std::move(dropped));
				player.addSkillCard(overflow.release());
			} else {
				cardSystem.discardSkill(std::move(overflow));
			}
		}
	}
}

void GameEngine::endTurn(Player& player) {
	if (gameState.getActivePlayer() == &player && gameState.getExtraRollAvailable()) {
		std::cout << "Masih mendapat bonus lempar karena double.\n";
		transactionLogger.log(gameState.getCurrentTurn(),
			player.getUsername(),
			"DOUBLE",
			"Masih harus lempar lagi karena double");
		return;
	}

	if (gameState.getActivePlayer() == &player &&
		!player.getHasRolledDiceThisTurn() &&
		!player.isBankrupt()) {
		std::cout << "Belum bisa mengakhiri giliran sebelum lempar dadu.\n";
		transactionLogger.log(gameState.getCurrentTurn(),
			player.getUsername(),
			"TURN_BLOCKED",
			"Tidak bisa selesai sebelum lempar dadu");
		return;
	}

	eventBus.publish(TurnEndedEvent(gameState.getCurrentTurn(), player.getUsername(), "Turn ended"));
	player.incrementTurnCount();
	player.resetTurnFlags();
	gameState.resetTurnFlags();
	turnManager.nextTurn();
	gameState.setCurrentTurn(turnManager.getTurnNumber());
	gameState.setActivePlayerIndex(turnManager.getCurrentPlayerIndex());
	turnPrepared = false;
}

void GameEngine::rollDice(Player& player) {
	if (gameState.getExtraRollAvailable()) {
		player.setHasUsedSkillCardThisTurn(false);
		gameState.setHasUsedSkillCard(false);
	}

	if (player.getHasRolledDiceThisTurn()) {
		std::cout << "Dadu sudah dilempar pada giliran ini.\n";
		return;
	}

	if (player.isJailed()) {
		handleJailTurn(player);
		return;
	}

	const bool manualRoll = dice.hasManualSet();
	const auto [d1, d2] = dice.rollPair();
	lastDiceTotal = d1 + d2;
	player.setHasRolledDiceThisTurn(true);
	gameState.setHasRolledDice(true);

	std::cout << "Hasil dadu: " << d1 << " + " << d2 << " = " << lastDiceTotal << "\n";
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "DADU",
		std::string(manualRoll ? "Lempar manual " : "Lempar ") +
		std::to_string(d1) + "+" + std::to_string(d2) + " = " + std::to_string(lastDiceTotal));

	const bool rolledDouble = d1 == d2;
	if (rolledDouble) {
		player.setConsecutiveDoubles(player.getConsecutiveDoubles() + 1);
		if (player.getConsecutiveDoubles() >= 3) {
			std::cout << "Double tiga kali. Masuk penjara.\n";
			gameState.setExtraRollAvailable(false);
			sendToJail(player);
			return;
		}
	} else {
		player.setConsecutiveDoubles(0);
		gameState.setExtraRollAvailable(false);
	}

	movePlayer(player, lastDiceTotal, true);
	resolveLanding(player, lastDiceTotal);

	if (rolledDouble && !player.isJailed() && !player.isBankrupt()) {
		player.setHasRolledDiceThisTurn(false);
		gameState.setHasRolledDice(false);
		player.setHasUsedSkillCardThisTurn(false);
		gameState.setHasUsedSkillCard(false);
		gameState.setExtraRollAvailable(true);
		transactionLogger.log(gameState.getCurrentTurn(),
			player.getUsername(),
			"DOUBLE",
			"Double " + std::to_string(d1) + "+" + std::to_string(d2) + ", lempar lagi");
		std::cout << "Double. " << player.getUsername() << " lempar lagi.\n";
	} else {
		gameState.setExtraRollAvailable(false);
	}
}

void GameEngine::movePlayer(Player& player, int steps, bool awardSalary) {
	const int size = board.getSize();
	if (size <= 0) {
		return;
	}
	const int start = player.getPosition();
	const int destination = (start + steps % size + size) % size;
	player.setPosition(destination);
	if (awardSalary && steps > 0 && start + steps >= size) {
		awardGoSalary(player);
	}
	Tile* tile = board.getTile(destination);
	std::cout << player.getUsername() << " bergerak ke " << (tile ? tile->getName() : "?") << ".\n";
}

void GameEngine::sendToJail(Player& player) {
	for (int i = 0; i < board.getSize(); ++i) {
		Tile* tile = board.getTile(i);
		if (tile && tile->getType() == TileType::JAIL) {
			player.setPosition(i);
			break;
		}
	}
	player.setStatus(PlayerStatus::JAILED);
	player.setConsecutiveDoubles(0);
	player.resetJailTurns();
	gameState.setExtraRollAvailable(false);
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PENJARA", "Masuk penjara");
}

void GameEngine::resolveLanding(Player& player, int diceTotal) {
	Tile* tile = board.getTile(player.getPosition());
	if (!tile || player.isBankrupt()) {
		return;
	}
	std::cout << "Mendarat di " << tile->getName() << " (" << tile->getCode() << ").\n";
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PETAK",
		"Mendarat di " + tile->getLabel());
	GameContext ctx{bank, festivalManager, transactionLogger, gameConfig, dice,
		const_cast<std::vector<Player*>&>(gameState.getPlayers()), board,
		gameState.getCurrentTurn(), &cardSystem, &auctionManager,
		&bankruptcyManager, &eventBus, &gameState, &turnManager};
	tile->onLand(player, ctx, diceTotal);
	checkBankruptcy(player);
}

void GameEngine::mortgageProperty(Player& player, const std::string& code) {
	PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property || property->getOwner() != &player || property->isMortgaged()) {
		std::cout << "Properti tidak dapat digadaikan.\n";
		return;
	}
	if (auto* street = dynamic_cast<StreetTile*>(property)) {
		ColorGroup* group = board.getColorGroup(street->getColor());
		bool hasBuildingsInGroup = false;
		if (group) {
			for (const StreetTile* member : group->getStreets()) {
				if (member && member->getBuildingLevel() > 0) {
					hasBuildingsInGroup = true;
					break;
				}
			}
		}
		if (hasBuildingsInGroup) {
			PlayerController* ctrl = player.getController();
			if (ctrl && !ctrl->confirmAction("Jual semua bangunan di color group ini? (y/n)")) {
				std::cout << "Gadai dibatalkan.\n";
				return;
			}
			bool soldBuildings = false;
			if (group) {
				for (StreetTile* member : group->getStreets()) {
					while (member && member->getBuildingLevel() > 0) {
						const Money refund(member->getBuildingSaleRefund());
						member->demolish();
						bank.payPlayer(player, refund, "Jual bangunan sebelum gadai");
						transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "LIKUIDASI",
							"Jual bangunan " + member->getLabel() + " " + refund.toString());
						soldBuildings = true;
					}
				}
				if (soldBuildings) {
					board.updateMonopolies();
				}
			}
		}
	}
	{
		PlayerController* ctrl = player.getController();
		if (ctrl && !ctrl->confirmAction("Lanjut menggadaikan " + property->getLabel() + "? (y/n)")) {
			std::cout << "Gadai dibatalkan.\n";
			return;
		}
	}
	property->setStatus(PropertyStatus::MORTGAGED);
	bank.payPlayer(player, property->getMortgageValue(), "Gadai");
	board.updateMonopolies();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "GADAI",
		"Gadai " + property->getLabel());
}

void GameEngine::redeemProperty(Player& player, const std::string& code) {
	PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property || property->getOwner() != &player || !property->isMortgaged()) {
		std::cout << "Properti tidak sedang digadaikan.\n";
		return;
	}
	const Money cost = property->getPrice();
	if (!player.canAfford(cost)) {
		std::cout << "Uang tidak cukup untuk tebus.\n";
		return;
	}
	bank.collectFromPlayer(player, cost, "Tebus");
	property->setStatus(PropertyStatus::OWNED);
	board.updateMonopolies();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "TEBUS",
		"Tebus " + property->getLabel() + " " + cost.toString());
}

void GameEngine::buildOnProperty(Player& player, const std::string& code) {
	PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	auto* street = dynamic_cast<StreetTile*>(property);
	if (!street || street->getOwner() != &player || street->isMortgaged()) {
		std::cout << "Petak tidak dapat dibangun.\n";
		return;
	}
	board.updateMonopolies();
	ColorGroup* group = board.getColorGroup(street->getColor());
	if (!group || !street->canBuild() || !group->satisfiesEvenBuildRule(*street)) {
		std::cout << "Syarat monopoli/pembangunan belum terpenuhi.\n";
		return;
	}

	const int level = street->getBuildingLevel();
	const Money cost = level < 4 ? street->getHouseCost() : street->getHotelCost();
	if (!player.canAfford(cost)) {
		std::cout << "Uang tidak cukup untuk bangun.\n";
		return;
	}
	bank.collectFromPlayer(player, cost, "Bangun");
	street->build();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "BANGUN",
		"Bangun di " + street->getLabel() + " level " + std::to_string(street->getBuildingLevel()));
}

void GameEngine::printBoard() const {
	std::cout << "=== Papan Nimonspoli (" << board.getSize() << " petak) ===\n";
	for (int i = 0; i < board.getSize(); ++i) {
		const Tile* tile = board.getTile(i);
		if (!tile) {
			std::cout << i + 1 << ". ? - ?\n";
			continue;
		}
		std::cout << i + 1 << ". " << tile->getCode() << " - " << tile->getName()
				  << " [" << tileTypeName(tile->getType()) << "]";

		if (const auto* property = dynamic_cast<const PropertyTile*>(tile)) {
			if (property->getOwner()) {
				std::cout << " Pemilik:" << property->getOwner()->getUsername();
				if (property->isMortgaged()) {
					std::cout << "(GADAI)";
				}
			} else {
				std::cout << " Pemilik:BANK";
			}
			if (const auto* street = dynamic_cast<const StreetTile*>(property)) {
				std::cout << " [" << colorName(street->getColor()) << "]";
				if (street->getBuildingLevel() > 0) {
					if (street->hasHotel()) {
						std::cout << " Hotel";
					} else {
						std::cout << " Rumah:" << street->getBuildingLevel();
					}
				}
			}
			// Festival info
			for (const FestivalEffectSnapshot& snap : festivalManager.getActiveEffectsSnapshot()) {
				if (snap.getProperty() == property && snap.getTurnsRemaining() > 0) {
					std::cout << " Festival:x" << snap.getMultiplier() << "(" << snap.getTurnsRemaining() << "t)";
					break;
				}
			}
		}

		// Player positions
		for (const Player* player : gameState.getPlayers()) {
			if (player && player->getPosition() == i && !player->isBankrupt()) {
				std::cout << " <" << player->getUsername() << (player->isJailed() ? ":JAIL" : "") << ">";
			}
		}
		std::cout << "\n";
	}
}

void GameEngine::printProperties(const Player& player) const {
	std::cout << "=== Properti " << player.getUsername() << " ===\n";
	if (player.getProperties().empty()) {
		std::cout << "(Tidak memiliki properti)\n";
		return;
	}
	int idx = 1;
	for (const PropertyTile* property : player.getProperties()) {
		if (!property) continue;
		std::cout << idx++ << ". " << property->getCode() << " - " << property->getName()
				  << " [" << tileTypeName(property->getType()) << "]";
		if (const auto* street = dynamic_cast<const StreetTile*>(property)) {
			std::cout << " Warna:" << colorName(street->getColor());
		}
		std::cout << " Harga:" << property->getPrice().toString()
				  << " Status:" << (property->isMortgaged() ? "GADAI" : "DIMILIKI");
		if (const auto* street = dynamic_cast<const StreetTile*>(property)) {
			if (street->hasHotel()) {
				std::cout << " Hotel";
			} else if (street->getBuildingLevel() > 0) {
				std::cout << " Rumah:" << street->getBuildingLevel();
			}
		}
		// Festival info
		for (const FestivalEffectSnapshot& snap : festivalManager.getActiveEffectsSnapshot()) {
			if (snap.getProperty() == property && snap.getTurnsRemaining() > 0) {
				std::cout << " Festival:x" << snap.getMultiplier() << "(" << snap.getTurnsRemaining() << "t)";
				break;
			}
		}
		std::cout << "\n";
	}
}

void GameEngine::printAkta(const std::string& code) const {
	const PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property) {
		std::cout << "Petak tidak ditemukan atau bukan properti.\n";
		return;
	}
	std::cout << "=== Akta " << property->getName() << " (" << property->getCode() << ") ===\n";
	std::cout << "Jenis     : " << tileTypeName(property->getType()) << "\n";
	std::cout << "Harga     : " << property->getPrice().toString() << "\n";
	std::cout << "Nilai Gadai: " << property->getMortgageValue().toString() << "\n";
	std::cout << "Pemilik   : " << (property->getOwner() ? property->getOwner()->getUsername() : "BANK") << "\n";
	std::cout << "Status    : " << (property->isMortgaged() ? "GADAI" : (property->getOwner() ? "DIMILIKI" : "TERSEDIA")) << "\n";

	if (const auto* street = dynamic_cast<const StreetTile*>(property)) {
		std::cout << "Warna     : " << colorName(street->getColor()) << "\n";
		const bool monopoly = street->isMonopolyComplete();
		std::cout << "Monopoli  : " << (monopoly ? "YA" : "TIDAK") << "\n";
		std::cout << "Bangunan  : ";
		if (street->hasHotel()) {
			std::cout << "HOTEL";
		} else if (street->getBuildingLevel() > 0) {
			std::cout << street->getBuildingLevel() << " Rumah";
		} else {
			std::cout << "Kosong";
		}
		std::cout << "\n";
		std::cout << "Biaya Rumah: " << street->getHouseCost().toString() << "\n";
		std::cout << "Biaya Hotel: " << street->getHotelCost().toString() << "\n";
		std::cout << "Tabel Sewa :\n";
		const auto& rents = street->getRentLevels();
		const std::vector<std::string> labels = {"  Tanah kosong", "  1 Rumah     ", "  2 Rumah     ", "  3 Rumah     ", "  4 Rumah     ", "  Hotel       "};
		for (std::size_t i = 0; i < rents.size() && i < labels.size(); ++i) {
			std::cout << labels[i] << ": " << Money(rents[i]).toString() << "\n";
		}
	} else if (property->getType() == TileType::RAILROAD) {
		std::cout << "Sewa bergantung jumlah Railroad yang dimiliki.\n";
		if (property->getOwner()) {
			const int count = property->getOwner()->countRailroads();
			std::cout << "Railroad dimiliki: " << count << " -> Sewa: " << property->getRent(0).toString() << "\n";
		}
	} else if (property->getType() == TileType::UTILITY) {
		std::cout << "Sewa = faktor pengali x total dadu.\n";
		if (property->getOwner()) {
			const int count = property->getOwner()->countUtilities();
			std::cout << "Utility dimiliki: " << count << "\n";
		}
	}

	// Festival info
	for (const FestivalEffectSnapshot& snap : festivalManager.getActiveEffectsSnapshot()) {
		if (snap.getProperty() == property && snap.getTurnsRemaining() > 0) {
			std::cout << "Festival  : Aktif x" << snap.getMultiplier()
					  << " (sisa " << snap.getTurnsRemaining() << " giliran)\n";
			break;
		}
	}
}

void GameEngine::printLog(int lastN) const {
	const std::vector<LogEntry> entries = lastN > 0 ? transactionLogger.getLastN(lastN) : transactionLogger.getFullLog();
	for (const LogEntry& entry : entries) {
		std::cout << "[Turn " << entry.turn << "] " << entry.username << " | " << entry.actionType
				  << " | " << entry.detail << "\n";
	}
}

void GameEngine::awardGoSalary(Player& player) {
	bank.payPlayer(player, Money(gameConfig.getGoSalary()), "GO");
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "GO", "Gaji " + Money(gameConfig.getGoSalary()).toString());
}

bool GameEngine::handleAuctionCommand(const std::string& input, Player& player) {
	std::istringstream stream(input);
	std::string command;
	stream >> command;
	command = lower(command);
	if (command != "lelang" && command != "auction") {
		return false;
	}

	std::string code;
	stream >> code;
	if (code.empty()) {
		std::cout << "Format: lelang <kode_properti>\n";
		return true;
	}

	PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property) {
		std::cout << "Properti tidak ditemukan.\n";
		return true;
	}

	if (property->getOwner() != nullptr) {
		std::cout << "Properti sudah memiliki pemilik, lelang ditolak.\n";
		return true;
	}

	auctionManager.runAuction(*property, player, gameState.getPlayers(), bank, board, transactionLogger, gameState.getCurrentTurn());
	return true;
}

void GameEngine::checkBankruptcy(Player& /*player*/) {
	// Pengecekan bangkrut sudah ditangani langsung di resolveLanding() dan executeBankruptcy().
}

std::string GameEngine::normalizeCommandToken(const std::string& input) const {
	std::istringstream stream(input);
	std::string token;
	stream >> token;
	return lower(token);
}

void GameEngine::payJailFine(Player& player) {
	if (!player.isJailed()) {
		std::cout << "Kamu tidak sedang di penjara.\n";
		return;
	}
	const Money fine(gameConfig.getJailFine());
	const Money due = player.applyOutgoingModifiers(fine);
	if (!player.canAfford(due)) {
		std::cout << "Uang tidak cukup untuk membayar denda.\n";
		return;
	}
	bank.collectFromPlayer(player, fine, "Denda penjara");
	player.setStatus(PlayerStatus::ACTIVE);
	player.resetJailTurns();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PENJARA", "Bayar denda");
}

void GameEngine::handleJailTurn(Player& player) {
	if (!player.isJailed()) {
		return;
	}

	gameState.setExtraRollAvailable(false);

	const Money fine(gameConfig.getJailFine());

	if (player.getJailTurnsRemaining() < 2) {
		if (PlayerController* controller = player.getController()) {
			if (controller->decideJailPay()) {
				const Money due = player.applyOutgoingModifiers(fine);
				if (!player.canAfford(due)) {
					std::cout << "Uang tidak cukup untuk membayar denda penjara.\n";
					bankruptcyManager.execute(player, nullptr, fine, gameState, board, bank, festivalManager, transactionLogger, turnManager, auctionManager, eventBus);
					return;
				}
				bank.collectFromPlayer(player, fine, "Denda penjara");
				player.setStatus(PlayerStatus::ACTIVE);
				player.resetJailTurns();
				transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PENJARA", "Bayar denda sebelum lempar");
			}
		}
	}

	if (player.getJailTurnsRemaining() >= 2) {
		const Money due = player.applyOutgoingModifiers(fine);
		if (!player.canAfford(due)) {
			std::cout << "Giliran ke-3 di penjara: tidak mampu membayar denda.\n";
			bankruptcyManager.execute(player, nullptr, fine, gameState, board, bank, festivalManager, transactionLogger, turnManager, auctionManager, eventBus);
			return;
		}
		bank.collectFromPlayer(player, fine, "Denda penjara (wajib)");
		player.setStatus(PlayerStatus::ACTIVE);
		player.resetJailTurns();
		transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PENJARA", "Denda wajib giliran ke-3");
	}

	const bool manualRoll = dice.hasManualSet();
	const auto [d1, d2] = dice.rollPair();
	lastDiceTotal = d1 + d2;
	player.setHasRolledDiceThisTurn(true);
	gameState.setHasRolledDice(true);

	std::cout << "Hasil dadu (penjara): " << d1 << " + " << d2 << " = " << lastDiceTotal << "\n";
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "DADU_PENJARA",
		std::string(manualRoll ? "Lempar penjara manual " : "Lempar penjara ") +
		std::to_string(d1) + "+" + std::to_string(d2) + " = " + std::to_string(lastDiceTotal));

	if (!player.isJailed()) {
		if (d1 == d2) {
			player.setConsecutiveDoubles(player.getConsecutiveDoubles() + 1);
			if (player.getConsecutiveDoubles() >= 3) {
				std::cout << "Double tiga kali. Masuk penjara.\n";
				sendToJail(player);
				return;
			}
		} else {
			player.setConsecutiveDoubles(0);
		}
		movePlayer(player, lastDiceTotal, true);
		resolveLanding(player, lastDiceTotal);
		checkBankruptcy(player);
		gameState.setExtraRollAvailable(false);
		return;
	}

	if (d1 == d2) {
		std::cout << "Double! Bebas dari penjara.\n";
		player.setStatus(PlayerStatus::ACTIVE);
		player.resetJailTurns();
		player.setConsecutiveDoubles(0);
		movePlayer(player, lastDiceTotal, false);
		resolveLanding(player, lastDiceTotal);
		checkBankruptcy(player);
		gameState.setExtraRollAvailable(false);
		return;
	}

	player.incrementJailTurn();
	std::cout << "Bukan double. Tetap di penjara.\n";
	endTurn(player);
}


