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

namespace {
	std::string lower(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return value;
	}

	std::string normalizeCode(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
		return value;
	}

	std::string displayName(std::string value) {
		std::replace(value.begin(), value.end(), '_', ' ');
		return value;
	}

	std::string tileLabel(const Tile& tile) {
		return tile.getName() + " (" + tile.getCode() + ")";
	}

	std::string propertyLabel(const PropertyTile& property) {
		return tileLabel(property);
	}

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

	std::vector<std::string> splitTokens(const std::string& line) {
		std::vector<std::string> tokens;
		std::istringstream stream(line);
		std::string token;
		while (stream >> token) {
			if (!token.empty() && token[0] == '#') {
				break;
			}
			tokens.push_back(token);
		}
		return tokens;
	}

	bool parseIntToken(const std::string& token, int& out) {
		try {
			std::size_t used = 0;
			const int value = std::stoi(token, &used);
			if (used != token.size()) {
				return false;
			}
			out = value;
			return true;
		} catch (...) {
			return false;
		}
	}

	Color parseColor(const std::string& raw) {
		const std::string value = lower(raw);
		if (value == "coklat") return Color::BROWN;
		if (value == "biru_muda") return Color::LIGHT_BLUE;
		if (value == "merah_muda" || value == "pink") return Color::PINK;
		if (value == "orange") return Color::ORANGE;
		if (value == "merah") return Color::RED;
		if (value == "kuning") return Color::YELLOW;
		if (value == "hijau") return Color::GREEN;
		if (value == "biru_tua") return Color::DARK_BLUE;
		if (value == "abu_abu") return Color::GRAY;
		return Color::DEFAULT;
	}

	std::string colorName(Color color) {
		switch (color) {
			case Color::BROWN: return "COKLAT";
			case Color::LIGHT_BLUE: return "BIRU MUDA";
			case Color::PINK: return "MERAH MUDA";
			case Color::ORANGE: return "ORANGE";
			case Color::RED: return "MERAH";
			case Color::YELLOW: return "KUNING";
			case Color::GREEN: return "HIJAU";
			case Color::DARK_BLUE: return "BIRU TUA";
			case Color::GRAY: return "ABU-ABU";
			default: return "DEFAULT";
		}
	}

	std::string tileTypeName(TileType type) {
		switch (type) {
			case TileType::STREET: return "Street";
			case TileType::RAILROAD: return "Railroad";
			case TileType::UTILITY: return "Utility";
			case TileType::CHANCE: return "Kesempatan";
			case TileType::COMMUNITY_CHEST: return "Dana Umum";
			case TileType::FESTIVAL: return "Festival";
			case TileType::TAX_PPH: return "PPH";
			case TileType::TAX_PBM: return "PBM";
			case TileType::GO: return "GO";
			case TileType::JAIL: return "Penjara";
			case TileType::FREE_PARKING: return "Bebas Parkir";
			case TileType::GO_TO_JAIL: return "Pergi ke Penjara";
		}
		return "Unknown";
	}

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

	PropertyInfo makePropertyInfo(const PropertyTile& property) {
		PropertyInfo info;
		info.code = property.getCode();
		info.name = property.getName();
		info.ownerName = property.getOwner() ? property.getOwner()->getUsername() : "";
		info.status = property.getStatus();
		info.buildingLevel = property.getBuildingLevel();
		info.purchasePrice = property.getPrice();
		return info;
	}

	bool fileExists(const std::string& path) {
		std::ifstream file(path);
		return static_cast<bool>(file);
	}

	int buildingSaleRefund(const StreetTile& street) {
		const int houses = std::min(street.getBuildingLevel(), 4);
		int refund = houses * street.getHouseCost().getAmount() / 2;
		if (street.hasHotel()) {
			refund += street.getHotelCost().getAmount() / 2;
		}
		return refund;
	}

	Money propertySaleValue(const PropertyTile& property) {
		if (property.isMortgaged()) {
			return Money::zero();
		}

		int total = property.getPrice().getAmount();
		if (const auto* street = dynamic_cast<const StreetTile*>(&property)) {
			total += buildingSaleRefund(*street);
		}
		return Money(total);
	}

	bool canMortgagePropertyNow(const PropertyTile& property, const Board& board) {
		if (property.isMortgaged()) {
			return false;
		}

		const auto* street = dynamic_cast<const StreetTile*>(&property);
		if (!street) {
			return true;
		}

		const ColorGroup* group = board.getColorGroup(street->getColor());
		if (!group) {
			return street->getBuildingLevel() == 0;
		}
		for (const StreetTile* member : group->getStreets()) {
			if (member && member->getBuildingLevel() > 0) {
				return false;
			}
		}
		return true;
	}

	bool satisfiesEvenBuildRule(const StreetTile& street, const Board& board) {
		const ColorGroup* group = board.getColorGroup(street.getColor());
		if (!group) {
			return true;
		}

		const int level = street.getBuildingLevel();
		if (level < 4) {
			return level == group->getMinBuildLevel();
		}
		if (level == 4) {
			return group->hasUniformBuildLevel();
		}
		return false;
	}

	void demolishAllBuildings(StreetTile& street) {
		while (street.getBuildingLevel() > 0) {
			street.demolish();
		}
	}

	int activeOtherPlayersCount(const std::vector<Player*>& players, const Player& current) {
		int count = 0;
		for (const Player* player : players) {
			if (player && player != &current && !player->isBankrupt()) {
				++count;
			}
		}
		return count;
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

	LiquidationState buildLiquidationStateSnapshot(const Player& debtor, const Board& board, Money obligation) {
		LiquidationState state;
		state.obligation = obligation;
		state.currentBalance = debtor.getMoney();
		state.maxLiquidation = debtor.getMoney();

		int optionIndex = 1;
		for (PropertyTile* property : debtor.getProperties()) {
			if (!property) {
				continue;
			}

			if (!property->isMortgaged()) {
				const Money sellValue = propertySaleValue(*property);
				if (sellValue.isPositive()) {
					LiquidationOption option;
					option.index = optionIndex++;
					option.type = LiquidationType::SELL;
					option.code = property->getCode();
					option.name = property->getName();
					option.value = sellValue;
					option.description = "Jual ke Bank";
					state.options.push_back(option);
					state.maxLiquidation += sellValue;
				}
			}

			if (canMortgagePropertyNow(*property, board)) {
				LiquidationOption option;
				option.index = optionIndex++;
				option.type = LiquidationType::MORTGAGE;
				option.code = property->getCode();
				option.name = property->getName();
				option.value = property->getMortgageValue();
				option.description = "Gadai";
				state.options.push_back(option);
			}
		}

		return state;
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
	gameState.setPlayers(players);
	gameState.setTurnOrder(players);
	gameState.setActivePlayerIndex(0);
	gameState.setCurrentTurn(1);
	gameState.setMaxTurn(maxTurn > 0 ? maxTurn : gameConfig.getMaxTurns());
	turnPrepared = false;

	turnManager.setPlayerCount(static_cast<int>(players.size()));
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
		if (gameState.getMaxTurn() > 0 && gameState.getCurrentTurn() > gameState.getMaxTurn()) {
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
	const std::vector<PlayerSummary> standing = buildPlayerSummaries(gameState.getPlayers());
	std::cout << "=== Peringkat kekayaan ===\n";
	int rank = 1;
	for (const PlayerSummary& summary : standing) {
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
			executeBankruptcy(player, nullptr, Money(0));
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

		if (command == "lempar" || command == "roll_dice") {
			std::string maybeDadu;
			stream >> maybeDadu;
			rollDice(player);
			return true;
		}

		if (command == "atur") {
			std::string dadu;
			int d1 = 0;
			int d2 = 0;
			stream >> dadu >> d1 >> d2;
			dice.setManual(d1, d2);
			transactionLogger.log(gameState.getCurrentTurn(),
				player.getUsername(),
				"ATUR_DADU",
				"Roll berikutnya: " + std::to_string(d1) + "+" + std::to_string(d2));
			return true;
		}

		if (command == "cetak" || command == "print") {
			std::string what;
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

		if (command == "gunakan" || command == "use_skill") {
			std::string maybeKemampuan;
			int index = 0;
			if (command == "gunakan") {
				stream >> maybeKemampuan >> index;
			} else {
				stream >> index;
			}
			useSkillCard(player, index - 1);
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

	eventBus.publish(TurnEndedEvent(gameState.getCurrentTurn(), player.getUsername(), "Turn ended"));
	player.resetTurnFlags();
	gameState.resetTurnFlags();
	turnManager.nextTurn();
	gameState.setCurrentTurn(turnManager.getTurnNumber());
	gameState.setActivePlayerIndex(turnManager.getCurrentPlayerIndex());
	turnPrepared = false;
}

void GameEngine::rollDice(Player& player) {
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
		"Mendarat di " + tileLabel(*tile));
	switch (tile->getType()) {
		case TileType::STREET:
		case TileType::RAILROAD:
		case TileType::UTILITY:
			resolvePropertyLanding(player, static_cast<PropertyTile&>(*tile), diceTotal);
			break;
		case TileType::TAX_PPH:
			resolveTaxLanding(player, true);
			break;
		case TileType::TAX_PBM:
			resolveTaxLanding(player, false);
			break;
		case TileType::CHANCE:
			resolveCardLanding(player, true);
			break;
		case TileType::COMMUNITY_CHEST:
			resolveCardLanding(player, false);
			break;
		case TileType::FESTIVAL:
			resolveFestivalLanding(player);
			break;
		case TileType::GO_TO_JAIL:
			sendToJail(player);
			break;
		case TileType::GO:
		case TileType::JAIL:
		case TileType::FREE_PARKING:
			break;
	}
	checkBankruptcy(player);
}

void GameEngine::resolveCardLanding(Player& player, bool chance) {
	GameContext context{bank, festivalManager, transactionLogger, gameConfig, dice, const_cast<std::vector<Player*>&>(gameState.getPlayers()), board, gameState.getCurrentTurn()};
	const auto resolvePeerPaymentCard = [&](const CardResult& rawResult) -> bool {
		if (rawResult.action != CardResultAction::RECEIVE_FROM_EACH_PLAYER &&
			rawResult.action != CardResultAction::PAY_EACH_PLAYER) {
			return false;
		}

		for (Player* other : gameState.getPlayers()) {
			if (!other || other == &player || other->isBankrupt()) {
				continue;
			}

			if (rawResult.action == CardResultAction::RECEIVE_FROM_EACH_PLAYER) {
				if (!other->canAfford(other->applyOutgoingModifiers(rawResult.amount))) {
					executeBankruptcy(*other, &player, rawResult.amount);
				} else {
					bank.transferBetweenPlayers(*other, player, rawResult.amount, rawResult.message);
				}
				continue;
			}

			if (player.isBankrupt()) {
				break;
			}
			if (!player.canAfford(player.applyOutgoingModifiers(rawResult.amount))) {
				executeBankruptcy(player, other, rawResult.amount);
			} else if (!player.isBankrupt()) {
				bank.transferBetweenPlayers(player, *other, rawResult.amount, rawResult.message);
			}
		}

		if (!rawResult.message.empty()) {
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU", rawResult.message);
		}
		return true;
	};

	if (chance) {
		auto card = cardSystem.drawChance();
		if (!card) return;
		std::cout << "Kartu Kesempatan: " << card->getDescription() << "\n";
		CardResult rawResult = card->execute(player, context);
		transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU",
			"Kesempatan: " + card->getDescription());
		if (resolvePeerPaymentCard(rawResult)) {
			cardSystem.discardChance(std::move(card));
			return;
		}
		CardResult result = CardSystem::applyImmediateResult(player, context, rawResult);
		cardSystem.discardChance(std::move(card));
		if (!result.success) {
			std::cout << result.message << "\n";
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU", result.message);
			if (rawResult.action == CardResultAction::PAY_BANK) {
				executeBankruptcy(player, nullptr, rawResult.amount);
			} else if (rawResult.action == CardResultAction::PAY_EACH_PLAYER) {
				executeBankruptcy(player, nullptr,
					Money(rawResult.amount.getAmount() * activeOtherPlayersCount(gameState.getPlayers(), player)));
			}
			return;
		}
		if (!result.message.empty()) {
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU", result.message);
		}
		if (result.success && result.resolveLanding) {
			resolveLanding(player, lastDiceTotal);
		}
	} else {
		auto card = cardSystem.drawCommunityChest();
		if (!card) return;
		std::cout << "Kartu Dana Umum: " << card->getDescription() << "\n";
		CardResult rawResult = card->execute(player, context);
		transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU",
			"Dana Umum: " + card->getDescription());
		if (resolvePeerPaymentCard(rawResult)) {
			cardSystem.discardCommunityChest(std::move(card));
			return;
		}
		CardResult result = CardSystem::applyImmediateResult(player, context, rawResult);
		cardSystem.discardCommunityChest(std::move(card));
		if (!result.success) {
			std::cout << result.message << "\n";
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU", result.message);
			if (rawResult.action == CardResultAction::PAY_BANK) {
				executeBankruptcy(player, nullptr, rawResult.amount);
			} else if (rawResult.action == CardResultAction::PAY_EACH_PLAYER) {
				executeBankruptcy(player, nullptr,
					Money(rawResult.amount.getAmount() * activeOtherPlayersCount(gameState.getPlayers(), player)));
			}
			return;
		}
		if (!result.message.empty()) {
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU", result.message);
		}
		if (result.success && result.resolveLanding) {
			resolveLanding(player, lastDiceTotal);
		}
	}
}

void GameEngine::resolvePropertyLanding(Player& player, PropertyTile& property, int diceTotal) {
	if (!property.getOwner()) {
		if (property.getType() == TileType::RAILROAD || property.getType() == TileType::UTILITY) {
			property.setOwner(&player);
			property.setStatus(PropertyStatus::OWNED);
			player.addProperty(&property);
			board.updateMonopolies();
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(),
				property.getType() == TileType::RAILROAD ? "RAILROAD" : "UTILITY",
				propertyLabel(property) + " kini dimiliki otomatis");
			return;
		}

		bool wantsBuy = false;
		if (PlayerController* controller = player.getController()) {
			wantsBuy = controller->decideBuyProperty(makePropertyInfo(property), player.getMoney());
		}
		if (wantsBuy && player.canAfford(property.getPrice())) {
			bank.collectFromPlayer(player, property.getPrice(), "Beli properti");
			property.setOwner(&player);
			property.setStatus(PropertyStatus::OWNED);
			player.addProperty(&property);
			board.updateMonopolies();
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "BELI",
				"Beli " + propertyLabel(property) + " " + property.getPrice().toString());
		} else {
			auctionProperty(property, player);
		}
		return;
	}

	if (property.getOwner() == &player || property.isMortgaged()) {
		return;
	}

	Player* owner = property.getOwner();
	Money rent = property.getRent(diceTotal);
	rent = owner->applyIncomingModifiers(rent, &property);
	if (!player.canAfford(player.applyOutgoingModifiers(rent))) {
		std::cout << player.getUsername() << " tidak mampu membayar sewa " << rent.toString() << ".\n";
		executeBankruptcy(player, owner, rent);
		return;
	}
	bank.transferBetweenPlayers(player, *owner, rent, "Sewa " + property.getCode());
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "SEWA",
		"Bayar sewa " + propertyLabel(property) + " " + rent.toString());
}

void GameEngine::resolveTaxLanding(Player& player, bool pph) {
	Money tax(0);
	std::string taxLabel = pph ? "pajak penghasilan" : "pajak barang mewah";
	if (pph) {
		TaxChoice choice = TaxChoice::FLAT;
		const int percentAmount = player.getTotalWealth().getAmount() * gameConfig.getPphPercentage() / 100;
		if (PlayerController* controller = player.getController()) {
			choice = controller->decideTax(gameConfig.getPphFlat(), percentAmount);
		}
		tax = choice == TaxChoice::FLAT ? Money(gameConfig.getPphFlat()) : Money(percentAmount);
	} else {
		tax = Money(gameConfig.getPbmFlat());
	}

	if (!player.canAfford(player.applyOutgoingModifiers(tax))) {
		executeBankruptcy(player, nullptr, tax);
		return;
	}
	bank.collectFromPlayer(player, tax, "Pajak");
	std::cout << player.getUsername() << " membayar " << taxLabel << " " << tax.toString() << ".\n";
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PAJAK",
		"Bayar " + taxLabel + " " + tax.toString());
}

void GameEngine::resolveFestivalLanding(Player& player) {
	std::vector<PropertyInfo> owned;
	for (PropertyTile* property : player.getProperties()) {
		if (property && !property->isMortgaged()) {
			owned.push_back(makePropertyInfo(*property));
		}
	}
	if (owned.empty()) {
		std::cout << "Tidak ada properti untuk festival.\n";
		return;
	}

	std::string code;
	if (PlayerController* controller = player.getController()) {
		code = controller->decideFestivalProperty(owned);
	}
	PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property || property->getOwner() != &player) {
		std::cout << "Properti festival tidak valid.\n";
		return;
	}
	FestivalResult result = festivalManager.applyFestival(player, *property);
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "FESTIVAL",
		"Aktifkan festival di " + propertyLabel(*property) + " x" + std::to_string(result.getMultiplier()));
}

void GameEngine::useSkillCard(Player& player, int cardIndex) {
	if (player.getHasRolledDiceThisTurn()) {
		std::cout << "Kartu kemampuan hanya bisa digunakan sebelum lempar dadu.\n";
		return;
	}
	if (gameState.getExtraRollAvailable()) {
		std::cout << "Bonus lempar harus diselesaikan dulu sebelum pakai kartu kemampuan.\n";
		return;
	}
	if (player.getHasUsedSkillCardThisTurn()) {
		std::cout << "Sudah menggunakan kartu kemampuan pada giliran ini.\n";
		return;
	}

	std::unique_ptr<SkillCard> card(player.removeSkillCard(cardIndex));
	if (!card) {
		std::cout << "Nomor kartu tidak valid.\n";
		return;
	}

	GameContext context{bank, festivalManager, transactionLogger, gameConfig, dice, const_cast<std::vector<Player*>&>(gameState.getPlayers()), board, gameState.getCurrentTurn()};
	CardResult result = card->activate(player, context);
	if (result.action == CardResultAction::TELEPORT) {
		std::string code = player.getController() ? player.getController()->decideTeleportTarget() : "";
		Tile* tile = board.getTileByCode(normalizeCode(code));
		result.destinationIndex = tile ? tile->getId() : -1;
	} else if (result.action == CardResultAction::LASSO) {
		std::vector<std::string> names;
		for (Player* other : gameState.getPlayers()) {
			if (other && other != &player && !other->isBankrupt()) names.push_back(other->getUsername());
		}
		std::string target = player.getController() ? player.getController()->decideLassoTarget(names) : "";
		for (Player* other : gameState.getPlayers()) {
			if (other && other->getUsername() == target) result.targetPlayer = other;
		}
	} else if (result.action == CardResultAction::DEMOLISH_PROPERTY) {
		std::vector<PropertyInfo> candidates;
		for (Player* other : gameState.getPlayers()) {
			if (!other || other == &player) continue;
			for (PropertyTile* property : other->getProperties()) {
				if (property) candidates.push_back(makePropertyInfo(*property));
			}
		}
		std::string code = player.getController() ? player.getController()->decideDemolitionTarget(candidates) : "";
		result.targetProperty = board.getPropertyByCode(normalizeCode(code));
	}

	CardResult applied = CardSystem::applyImmediateResult(player, context, result);
	if (!applied.success) {
		std::cout << applied.message << "\n";
		player.addSkillCard(card.release());
		return;
	}

	player.setHasUsedSkillCardThisTurn(true);
	gameState.setHasUsedSkillCard(true);
	cardSystem.discardSkill(std::move(card));
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU",
		result.message.empty() ? "Menggunakan kartu kemampuan" : result.message);
	if (applied.resolveLanding) {
		resolveLanding(player, lastDiceTotal);
	}
}

void GameEngine::mortgageProperty(Player& player, const std::string& code) {
	PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property || property->getOwner() != &player || property->isMortgaged()) {
		std::cout << "Properti tidak dapat digadaikan.\n";
		return;
	}
	if (auto* street = dynamic_cast<StreetTile*>(property)) {
		ColorGroup* group = board.getColorGroup(street->getColor());
		if (group) {
			bool soldBuildings = false;
			for (StreetTile* member : group->getStreets()) {
				while (member && member->getBuildingLevel() > 0) {
					const Money refund(buildingSaleRefund(*member));
					member->demolish();
					bank.payPlayer(player, refund, "Jual bangunan sebelum gadai");
					transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "LIKUIDASI",
						"Jual bangunan " + propertyLabel(*member) + " " + refund.toString());
					soldBuildings = true;
				}
			}
			if (soldBuildings) {
				board.updateMonopolies();
			}
		}
	}
	property->setStatus(PropertyStatus::MORTGAGED);
	bank.payPlayer(player, property->getMortgageValue(), "Gadai");
	board.updateMonopolies();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "GADAI",
		"Gadai " + propertyLabel(*property));
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
		"Tebus " + propertyLabel(*property) + " " + cost.toString());
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
	if (!group || !street->canBuild() || !satisfiesEvenBuildRule(*street, board)) {
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
		"Bangun di " + propertyLabel(*street) + " level " + std::to_string(street->getBuildingLevel()));
}

void GameEngine::printBoard() const {
	std::cout << "=== Papan Nimonspoli ===\n";
	for (int i = 0; i < board.getSize(); ++i) {
		const Tile* tile = board.getTile(i);
		std::cout << i + 1 << ". " << (tile ? tile->getCode() : "?") << " - "
				  << (tile ? tile->getName() : "?") << " [" << (tile ? tileTypeName(tile->getType()) : "?") << "]";
		for (const Player* player : gameState.getPlayers()) {
			if (player && player->getPosition() == i && !player->isBankrupt()) {
				std::cout << " (" << player->getUsername() << (player->isJailed() ? ":IN" : "") << ")";
			}
		}
		std::cout << "\n";
	}
}

void GameEngine::printProperties(const Player& player) const {
	std::cout << "=== Properti " << player.getUsername() << " ===\n";
	for (const PropertyTile* property : player.getProperties()) {
		if (!property) continue;
		std::cout << property->getCode() << " " << property->getName()
				  << " " << (property->isMortgaged() ? "MORTGAGED" : "OWNED")
				  << " bangunan=" << property->getBuildingLevel() << "\n";
	}
}

void GameEngine::printAkta(const std::string& code) const {
	const PropertyTile* property = board.getPropertyByCode(normalizeCode(code));
	if (!property) {
		std::cout << "Petak tidak ditemukan atau bukan properti.\n";
		return;
	}
	std::cout << "=== Akta " << property->getName() << " (" << property->getCode() << ") ===\n";
	std::cout << "Harga: " << property->getPrice().toString() << "\n";
	std::cout << "Gadai: " << property->getMortgageValue().toString() << "\n";
	std::cout << "Status: " << (property->getOwner() ? property->getOwner()->getUsername() : "BANK") << "\n";
	if (auto* street = dynamic_cast<const StreetTile*>(property)) {
		std::cout << "Warna: " << colorName(street->getColor()) << "\n";
		std::cout << "Bangunan: " << street->getBuildingLevel() << "\n";
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

bool GameEngine::auctionProperty(PropertyTile& property, Player& trigger) {
	std::cout << "Lelang " << property.getName() << " dimulai.\n";
	Player* winner = nullptr;
	int highestBid = -1;
	int passCount = 0;
	const auto& players = gameState.getPlayers();
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
				bid = highestBid + 1;
			}
			if (bid > highestBid && bidder->canAfford(Money(bid))) {
				highestBid = bid;
				winner = bidder;
				passCount = 0;
				transactionLogger.log(gameState.getCurrentTurn(), bidder->getUsername(), "LELANG_BID",
					"Bid " + Money(bid).toString() + " untuk " + propertyLabel(property));
				continue;
			}
		}

		if (!winner && passCount + 1 >= activeParticipants) {
			const int forcedBid = std::max(0, highestBid + 1);
			if (bidder->canAfford(Money(forcedBid))) {
				highestBid = forcedBid;
				winner = bidder;
				passCount = 0;
				transactionLogger.log(gameState.getCurrentTurn(), bidder->getUsername(), "LELANG_BID",
					"Bid paksa " + Money(forcedBid).toString() + " untuk " + propertyLabel(property));
				continue;
			}
		}

		transactionLogger.log(gameState.getCurrentTurn(), bidder->getUsername(), "LELANG_PASS",
			"Pass pada lelang " + propertyLabel(property));
		++passCount;
		if (winner && passCount >= passThreshold) {
			break;
		}
	}

	if (!winner) {
		std::cout << "Lelang selesai tanpa pemenang.\n";
		return false;
	}

	bank.collectFromPlayer(*winner, Money(highestBid), "Lelang");
	property.setOwner(winner);
	property.setStatus(PropertyStatus::OWNED);
	winner->addProperty(&property);
	board.updateMonopolies();
	transactionLogger.log(gameState.getCurrentTurn(), winner->getUsername(), "LELANG",
		"Menang lelang " + propertyLabel(property) + " " + Money(highestBid).toString());
	std::cout << "Pemenang lelang: " << winner->getUsername() << "\n";
	return true;
}

bool GameEngine::handleAuctionCommand(const std::string& input, Player& player) {
	(void)input;
	(void)player;
	return false;
}

void GameEngine::checkBankruptcy(Player& player) {
	(void)player;
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
					executeBankruptcy(player, nullptr, fine);
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
			executeBankruptcy(player, nullptr, fine);
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

void GameEngine::executeBankruptcy(Player& debtor, Player* creditor, Money obligation) {
	if (debtor.isBankrupt()) {
		return;
	}

	auto applyLiquidationOption = [&](const LiquidationOption& option) -> bool {
		PropertyTile* property = board.getPropertyByCode(normalizeCode(option.code));
		if (!property || property->getOwner() != &debtor) {
			return false;
		}

		if (option.type == LiquidationType::SELL) {
			if (property->isMortgaged()) {
				return false;
			}

			const Money value = propertySaleValue(*property);
			festivalManager.removeEffectsForProperty(property);
			if (auto* street = dynamic_cast<StreetTile*>(property)) {
				demolishAllBuildings(*street);
			}
			debtor.removeProperty(property);
			property->setOwner(nullptr);
			property->setStatus(PropertyStatus::BANK);
			bank.payPlayer(debtor, value, "Likuidasi jual ke bank");
			board.updateMonopolies();
			transactionLogger.log(gameState.getCurrentTurn(), debtor.getUsername(), "LIKUIDASI",
				"Jual " + propertyLabel(*property) + " ke Bank " + value.toString());
			return true;
		}

		if (option.type == LiquidationType::MORTGAGE) {
			if (!canMortgagePropertyNow(*property, board)) {
				return false;
			}

			property->setStatus(PropertyStatus::MORTGAGED);
			bank.payPlayer(debtor, property->getMortgageValue(), "Likuidasi gadai");
			board.updateMonopolies();
			transactionLogger.log(gameState.getCurrentTurn(), debtor.getUsername(), "LIKUIDASI",
				"Gadai " + propertyLabel(*property) + " " + property->getMortgageValue().toString());
			return true;
		}

		return false;
	};

	if (obligation.isPositive()) {
		LiquidationState liquidationState = buildLiquidationStateSnapshot(debtor, board, obligation);
		if (liquidationState.maxLiquidation >= obligation) {
			transactionLogger.log(gameState.getCurrentTurn(), debtor.getUsername(), "LIKUIDASI",
				"Wajib likuidasi untuk menutup kewajiban " + obligation.toString());
			while (!debtor.canAfford(obligation)) {
				liquidationState = buildLiquidationStateSnapshot(debtor, board, obligation);
				if (liquidationState.options.empty()) {
					break;
				}

				int choice = 0;
				if (PlayerController* controller = debtor.getController()) {
					choice = controller->decideLiquidation(liquidationState);
				}

				const LiquidationOption* selected = nullptr;
				for (const LiquidationOption& option : liquidationState.options) {
					if (option.index == choice) {
						selected = &option;
						break;
					}
				}
				if (!selected) {
					selected = &*std::max_element(
						liquidationState.options.begin(),
						liquidationState.options.end(),
						[](const LiquidationOption& a, const LiquidationOption& b) {
							return a.value < b.value;
						});
				}
				if (!selected || !applyLiquidationOption(*selected)) {
					break;
				}
			}

			if (debtor.canAfford(obligation)) {
				if (creditor && !creditor->isBankrupt() && creditor != &debtor) {
					bank.transferBetweenPlayers(debtor, *creditor, obligation, "Pelunasan likuidasi");
					transactionLogger.log(gameState.getCurrentTurn(), debtor.getUsername(), "LIKUIDASI",
						"Lunas ke " + creditor->getUsername() + " " + obligation.toString());
				} else {
					bank.collectFromPlayer(debtor, obligation, "Pelunasan likuidasi");
					transactionLogger.log(gameState.getCurrentTurn(), debtor.getUsername(), "LIKUIDASI",
						"Lunas ke Bank " + obligation.toString());
				}
				return;
			}
		}
	}

	debtor.setStatus(PlayerStatus::BANKRUPT);
	eventBus.publish(BankruptcyEvent(gameState.getCurrentTurn(), debtor.getUsername(), "Bangkrut"));

	const std::vector<PropertyTile*> props(debtor.getProperties());
	transactionLogger.log(gameState.getCurrentTurn(), debtor.getUsername(), "BANGKRUT",
		creditor && !creditor->isBankrupt() && creditor != &debtor
			? ("Bangkrut ke " + creditor->getUsername())
			: "Bangkrut ke Bank");

	if (creditor && !creditor->isBankrupt() && creditor != &debtor) {
		const Money remainingCash = debtor.getMoney();
		if (!remainingCash.isZero()) {
			bank.transferBetweenPlayers(debtor, *creditor, remainingCash, "Aset bangkrut");
		}

		for (PropertyTile* prop : props) {
			if (!prop) {
				continue;
			}
			festivalManager.removeEffectsForProperty(prop);
			debtor.removeProperty(prop);
			prop->setOwner(creditor);
			creditor->addProperty(prop);
		}
	} else {
		const Money remainingCash = debtor.getMoney();
		if (!remainingCash.isZero()) {
			debtor.deductMoney(remainingCash);
		}

		for (PropertyTile* prop : props) {
			if (!prop) {
				continue;
			}
			festivalManager.removeEffectsForProperty(prop);
			if (auto* street = dynamic_cast<StreetTile*>(prop)) {
				demolishAllBuildings(*street);
			}
			debtor.removeProperty(prop);
			prop->setOwner(nullptr);
			prop->setStatus(PropertyStatus::BANK);
		}

		board.updateMonopolies();
		for (PropertyTile* prop : props) {
			if (prop) {
				auctionProperty(*prop, debtor);
			}
		}
	}

	board.updateMonopolies();

	std::vector<Player*> order(gameState.getTurnOrder().begin(), gameState.getTurnOrder().end());
	int removedIdx = -1;
	for (std::size_t i = 0; i < order.size(); ++i) {
		if (order[i] == &debtor) {
			removedIdx = static_cast<int>(i);
			order.erase(order.begin() + static_cast<std::ptrdiff_t>(i));
			break;
		}
	}
	if (removedIdx >= 0) {
		turnManager.removePlayer(removedIdx);
		gameState.setTurnOrder(order);
		gameState.setActivePlayerIndex(turnManager.getCurrentPlayerIndex());
	}
	turnPrepared = false;
}
