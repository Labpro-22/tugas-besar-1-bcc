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

}

GameEngine::GameEngine(GameState initialState)
	: gameState(initialState), running(false), turnPrepared(false), lastDiceTotal(0), commandRegistry(),
	  turnManager(), auctionManager(), bankruptcyManager(), eventBus(), transactionLogger(), bank(),
	  festivalManager(), gameConfig(), dice(), board(), cardSystem(), configDirectory("config"),
	  repository(nullptr) {
	bankruptcyManager.setBankruptcyThreshold(0);
	eventBus.subscribe(&transactionLogger);
	loadConfiguration(configDirectory);
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

bool GameEngine::loadConfiguration(const std::string& directory) {
	configDirectory = directory.empty() ? "config" : directory;
	const bool loaded = gameConfig.loadFromDirectory(configDirectory.c_str());
	buildDefaultBoard(configDirectory);
	return loaded;
}

void GameEngine::buildDefaultBoard(const std::string& directory) {
	board.clear();
	std::map<int, std::unique_ptr<Tile>> configuredTiles;

	std::ifstream file(directory + "/property.txt");
	std::string line;
	std::getline(file, line);
	while (std::getline(file, line)) {
		if (line.empty()) {
			continue;
		}

		std::istringstream stream(line);
		int id = 0;
		std::string code;
		std::string name;
		std::string kind;
		std::string colorText;
		int price = 0;
		int mortgage = 0;
		if (!(stream >> id >> code >> name >> kind >> colorText >> price >> mortgage)) {
			continue;
		}

		const std::string normalizedKind = lower(kind);
		if (normalizedKind == "street") {
			int houseCost = 0;
			int hotelCost = 0;
			stream >> houseCost >> hotelCost;
			std::vector<int> parsedRents;
			std::string token;
			while (stream >> token) {
				int value = 0;
				if (parseIntToken(token, value)) {
					parsedRents.push_back(value);
				}
			}

			auto street = std::make_unique<StreetTile>(
				id - 1,
				normalizeCode(code),
				displayName(name),
				Money(price),
				Money(mortgage),
				parseColor(colorText),
				Money(houseCost),
				Money(hotelCost),
				completeRentLevels(parsedRents)
			);
			ColorGroup* group = board.ensureColorGroup(street->getColor());
			group->addStreet(street.get());
			configuredTiles[id - 1] = std::move(street);
		} else if (normalizedKind == "railroad") {
			configuredTiles[id - 1] = std::make_unique<RailroadTile>(
				id - 1, normalizeCode(code), displayName(name), Money(price), Money(mortgage)
			);
		} else if (normalizedKind == "utility") {
			configuredTiles[id - 1] = std::make_unique<UtilityTile>(
				id - 1, normalizeCode(code), displayName(name), Money(price), Money(mortgage)
			);
		}
	}

	for (int i = 0; i < Board::BOARD_SIZE; ++i) {
		auto configured = configuredTiles.find(i);
		if (configured != configuredTiles.end()) {
			board.addTile(std::move(configured->second));
			continue;
		}

		switch (i + 1) {
			case 1:
				board.addTile(std::make_unique<GoTile>(i, "GO", "Petak Mulai", gameConfig.getGoSalary()));
				break;
			case 3:
			case 18:
				board.addTile(std::make_unique<CommunityChestTile>(i, "DNU", "Dana Umum"));
				break;
			case 5:
				board.addTile(std::make_unique<PPHTile>(i, "PPH", "Pajak Penghasilan", gameConfig.getPphFlat(), gameConfig.getPphPercentage()));
				break;
			case 8:
			case 34:
				board.addTile(std::make_unique<FestivalTile>(i, "FES", "Festival"));
				break;
			case 11:
				board.addTile(std::make_unique<JailTile>(i, "PEN", "Penjara", gameConfig.getJailFine()));
				break;
			case 21:
				board.addTile(std::make_unique<FreeParkingTile>(i, "BBP", "Bebas Parkir"));
				break;
			case 23:
			case 37:
				board.addTile(std::make_unique<ChanceTile>(i, "KSP", "Kesempatan"));
				break;
			case 31:
				board.addTile(std::make_unique<GoToJailTile>(i, "PPJ", "Pergi ke Penjara"));
				break;
			case 39:
				board.addTile(std::make_unique<PBMTile>(i, "PBM", "Pajak Barang Mewah", gameConfig.getPbmFlat()));
				break;
			default:
				board.addTile(std::make_unique<FreeParkingTile>(i, "UNK", "Kosong"));
				break;
		}
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
	if (!repository) {
		return false;
	}
	return repository->save(gameState, board, transactionLogger, festivalManager, id);
}

bool GameEngine::loadGame(const std::string& id) {
	if (!repository) {
		return false;
	}
	if (!repository->loadInto(gameState, board, transactionLogger, festivalManager, id)) {
		return false;
	}
	syncTurnManagerAfterLoad();
	return true;
}

void GameEngine::syncTurnManagerAfterLoad() {
	turnManager.setPlayerCount(static_cast<int>(gameState.getTurnOrder().size()));
	turnManager.setTurnNumber(gameState.getCurrentTurn());
	turnManager.setCurrentPlayerIndex(gameState.getActivePlayerIndex());
	turnPrepared = false;
}

void GameEngine::printWinners() const {
	std::vector<const Player*> standing;
	for (Player* p : gameState.getPlayers()) {
		if (p && !p->isBankrupt()) {
			standing.push_back(p);
		}
	}
	std::sort(standing.begin(), standing.end(), [](const Player* a, const Player* b) {
		if (!a || !b) return a != nullptr;
		const Money wa = a->getTotalWealth();
		const Money wb = b->getTotalWealth();
		if (wa.getAmount() != wb.getAmount()) {
			return wa.getAmount() > wb.getAmount();
		}
		return a->getUsername() < b->getUsername();
	});
	std::cout << "=== Peringkat kekayaan ===\n";
	int rank = 1;
	for (const Player* p : standing) {
		if (!p) continue;
		std::cout << rank++ << ". " << p->getUsername() << " — " << p->getTotalWealth().toString() << "\n";
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
			if (!gameState.canSave()) {
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
			rollDice(player);
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
		try {
			player.addSkillCard(card.release());
		} catch (const CardSlotFullException&) {
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

	const auto [d1, d2] = dice.rollPair();
	lastDiceTotal = d1 + d2;
	player.setHasRolledDiceThisTurn(true);
	gameState.setHasRolledDice(true);

	std::cout << "Hasil dadu: " << d1 << " + " << d2 << " = " << lastDiceTotal << "\n";
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "DADU",
		"Lempar: " + std::to_string(d1) + "+" + std::to_string(d2) + "=" + std::to_string(lastDiceTotal));

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
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PENJARA", "Masuk penjara");
}

void GameEngine::resolveLanding(Player& player, int diceTotal) {
	Tile* tile = board.getTile(player.getPosition());
	if (!tile || player.isBankrupt()) {
		return;
	}

	std::cout << "Mendarat di " << tile->getName() << " (" << tile->getCode() << ").\n";
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
	if (chance) {
		auto card = cardSystem.drawChance();
		if (!card) return;
		std::cout << "Kartu Kesempatan: " << card->getDescription() << "\n";
		CardResult result = CardSystem::applyImmediateResult(player, context, card->execute(player, context));
		cardSystem.discardChance(std::move(card));
		if (result.success && result.resolveLanding) {
			resolveLanding(player, lastDiceTotal);
		}
	} else {
		auto card = cardSystem.drawCommunityChest();
		if (!card) return;
		std::cout << "Kartu Dana Umum: " << card->getDescription() << "\n";
		CardResult result = CardSystem::applyImmediateResult(player, context, card->execute(player, context));
		cardSystem.discardCommunityChest(std::move(card));
		if (result.success && result.resolveLanding) {
			resolveLanding(player, lastDiceTotal);
		}
	}
}

void GameEngine::resolvePropertyLanding(Player& player, PropertyTile& property, int diceTotal) {
	if (!property.getOwner()) {
		if (property.getType() == TileType::RAILROAD || property.getType() == TileType::UTILITY) {
			if (player.canAfford(property.getPrice())) {
				bank.collectFromPlayer(player, property.getPrice(), "Akuisisi otomatis");
				property.setOwner(&player);
				property.setStatus(PropertyStatus::OWNED);
				player.addProperty(&property);
				board.updateMonopolies();
				std::cout << property.getName() << " kini milik " << player.getUsername() << ".\n";
			}
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
			transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "BELI", property.getCode());
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
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "SEWA", property.getCode() + " " + rent.toString());
}

void GameEngine::resolveTaxLanding(Player& player, bool pph) {
	Money tax(0);
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
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "PAJAK", tax.toString());
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
		property->getCode() + " x" + std::to_string(result.getMultiplier()));
}

void GameEngine::useSkillCard(Player& player, int cardIndex) {
	if (player.getHasRolledDiceThisTurn()) {
		std::cout << "Kartu kemampuan hanya bisa digunakan sebelum lempar dadu.\n";
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
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "KARTU", result.message);
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
			for (StreetTile* member : group->getStreets()) {
				if (member->getBuildingLevel() > 0) {
					std::cout << "Jual bangunan color group dulu sebelum gadai.\n";
					return;
				}
			}
		}
	}
	property->setStatus(PropertyStatus::MORTGAGED);
	bank.payPlayer(player, property->getMortgageValue(), "Gadai");
	board.updateMonopolies();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "GADAI", property->getCode());
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
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "TEBUS", property->getCode());
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
	if (!group || !street->canBuild()) {
		std::cout << "Syarat monopoli/pembangunan belum terpenuhi.\n";
		return;
	}

	const int level = street->getBuildingLevel();
	for (StreetTile* member : group->getStreets()) {
		if (member == street) continue;
		if (level >= 4) {
			if (member->getBuildingLevel() < 4) {
				std::cout << "Semua properti group harus punya 4 rumah sebelum hotel.\n";
				return;
			}
		} else if (member->getBuildingLevel() < level) {
			std::cout << "Bangunan harus merata dalam color group.\n";
			return;
		}
	}

	const Money cost = level < 4 ? street->getHouseCost() : street->getHotelCost();
	if (!player.canAfford(cost)) {
		std::cout << "Uang tidak cukup untuk bangun.\n";
		return;
	}
	bank.collectFromPlayer(player, cost, "Bangun");
	street->build();
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "BANGUN", street->getCode());
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
	int index = 0;
	for (int i = 0; i < total; ++i) {
		if (players[i] == &trigger) {
			index = (i + 1) % total;
			break;
		}
	}

	while (passCount < std::max(1, total - 1)) {
		Player* bidder = players[index];
		index = (index + 1) % total;
		if (!bidder || bidder->isBankrupt()) {
			continue;
		}
		AuctionDecision decision;
		if (PlayerController* controller = bidder->getController()) {
			decision = controller->decideAuction(highestBid, bidder->getMoney());
		}
		if (action == AuctionAction::BID) {
			int bid = highestBid + 1;
			std::cout << "Bid untuk " << bidder->getUsername() << " (mis. lelang 500): ";
			std::string cmd;
			if (PlayerController* c = bidder->getController()) {
				cmd = c->chooseCommand(gameState.toView());
			}
			std::istringstream bs(cmd);
			std::string tok;
			if (bs >> tok) {
				const std::string lt = lower(tok);
				if (lt == "lelang" || lt == "bid") {
					bs >> bid;
				} else {
					std::istringstream bs2(cmd);
					bs2 >> bid;
				}
			}
			if (bid > highestBid && bidder->canAfford(Money(bid))) {
				highestBid = bid;
				winner = bidder;
				passCount = 0;
				continue;
			}
		}
		++passCount;
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
	transactionLogger.log(gameState.getCurrentTurn(), winner->getUsername(), "LELANG", property.getCode() + " " + std::to_string(highestBid));
	std::cout << "Pemenang lelang: " << winner->getUsername() << "\n";
	return true;
}

bool GameEngine::handleAuctionCommand(const std::string& input, Player& player) {
	(void)input;
	(void)player;
	return false;
}

void GameEngine::checkBankruptcy(Player& player) {
	if (bankruptcyManager.isBankrupt(player.getMoney().getAmount())) {
		player.setStatus(PlayerStatus::BANKRUPT);
		eventBus.publish(BankruptcyEvent(gameState.getCurrentTurn(), player.getUsername(), "Player bankrupt"));
	}
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

	const Money fine(gameConfig.getJailFine());

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

	const auto [d1, d2] = dice.rollPair();
	lastDiceTotal = d1 + d2;
	player.setHasRolledDiceThisTurn(true);
	gameState.setHasRolledDice(true);

	std::cout << "Hasil dadu (penjara): " << d1 << " + " << d2 << " = " << lastDiceTotal << "\n";
	transactionLogger.log(gameState.getCurrentTurn(), player.getUsername(), "DADU_PENJARA",
		std::to_string(d1) + "+" + std::to_string(d2) + "=" + std::to_string(lastDiceTotal));

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
		return;
	}

	player.incrementJailTurn();
	std::cout << "Bukan double. Tetap di penjara.\n";
	endTurn(player);
}

void GameEngine::executeBankruptcy(Player& debtor, Player* creditor, Money) {
	if (debtor.isBankrupt()) {
		return;
	}

	debtor.setStatus(PlayerStatus::BANKRUPT);
	eventBus.publish(BankruptcyEvent(gameState.getCurrentTurn(), debtor.getUsername(), "Bangkrut"));

	const std::vector<PropertyTile*> props(debtor.getProperties());
	for (PropertyTile* prop : props) {
		if (!prop) {
			continue;
		}
		festivalManager.removeEffectsForProperty(prop);
		if (auto* street = dynamic_cast<StreetTile*>(prop)) {
			while (street->getBuildingLevel() > 0) {
				street->demolish();
			}
		}
		debtor.removeProperty(prop);
		prop->setOwner(nullptr);
		prop->setStatus(PropertyStatus::BANK);
	}

	board.updateMonopolies();

	for (PropertyTile* prop : props) {
		if (!prop) {
			continue;
		}
		if (creditor && !creditor->isBankrupt() && creditor != &debtor) {
			prop->setOwner(creditor);
			prop->setStatus(PropertyStatus::OWNED);
			creditor->addProperty(prop);
		} else {
			Player* trigger = nullptr;
			for (Player* p : gameState.getTurnOrder()) {
				if (p && !p->isBankrupt() && p != &debtor) {
					trigger = p;
					break;
				}
			}
			if (!trigger) {
				for (Player* p : gameState.getPlayers()) {
					if (p && !p->isBankrupt() && p != &debtor) {
						trigger = p;
						break;
					}
				}
			}
			if (trigger) {
				auctionProperty(*prop, *trigger);
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
