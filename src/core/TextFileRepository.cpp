#include "core/TextFileRepository.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

#include "core/FestivalManager.hpp"
#include "core/log/header/TransactionLogger.hpp"
#include "core/state/header/GameState.hpp"
#include "models/Money.hpp"
#include "models/Player.hpp"
#include "models/board/header/Board.hpp"
#include "models/cards/CardSystem.hpp"
#include "models/cards/chance/ChanceGoToJailCard.hpp"
#include "models/cards/chance/ChanceGoToNearestStationCard.hpp"
#include "models/cards/chance/ChanceMoveBackThreeCard.hpp"
#include "models/cards/community/BirthdayCard.hpp"
#include "models/cards/community/DoctorFeeCard.hpp"
#include "models/cards/community/ElectionCampaignCard.hpp"
#include "models/cards/skill/DemolitionCard.hpp"
#include "models/cards/skill/DiscountCard.hpp"
#include "models/cards/skill/LassoCard.hpp"
#include "models/cards/skill/MoveCard.hpp"
#include "models/cards/skill/ShieldCard.hpp"
#include "models/cards/skill/TeleportCard.hpp"
#include "models/effects/DiscountEffect.hpp"
#include "models/effects/Effect.hpp"
#include "models/effects/ShieldEffect.hpp"
#include "tile/header/PropertyTile.hpp"
#include "tile/header/StreetTile.hpp"
#include "tile/header/Tile.hpp"
#include "utils/Enums.hpp"
#include "utils/Types.hpp"

namespace {

std::string toUpper(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
	return value;
}

std::string playerStatusToken(PlayerStatus s) {
	switch (s) {
		case PlayerStatus::ACTIVE: return "ACTIVE";
		case PlayerStatus::BANKRUPT: return "BANKRUPT";
		case PlayerStatus::JAILED: return "JAILED";
	}
	return "ACTIVE";
}

PlayerStatus parsePlayerStatus(const std::string& t) {
	if (t == "BANKRUPT") return PlayerStatus::BANKRUPT;
	if (t == "JAILED") return PlayerStatus::JAILED;
	return PlayerStatus::ACTIVE;
}

std::string propertyStatusToken(PropertyStatus s) {
	switch (s) {
		case PropertyStatus::BANK: return "BANK";
		case PropertyStatus::OWNED: return "OWNED";
		case PropertyStatus::MORTGAGED: return "MORTGAGED";
	}
	return "BANK";
}

PropertyStatus parsePropertyStatus(const std::string& t) {
	if (t == "OWNED") return PropertyStatus::OWNED;
	if (t == "MORTGAGED") return PropertyStatus::MORTGAGED;
	return PropertyStatus::BANK;
}

std::string propertyKindLower(const PropertyTile& p) {
	switch (p.getType()) {
		case TileType::STREET: return "street";
		case TileType::RAILROAD: return "railroad";
		case TileType::UTILITY: return "utility";
		default: return "other";
	}
}

std::string cardSaveLine(const SkillCard* c) {
	if (!c) return "";
	std::string line = c->getId();
	const std::string val = c->getSaveValue();
	const std::string dur = c->getSaveDuration();
	if (!val.empty()) {
		line += ' ';
		line += val;
		if (!dur.empty()) {
			line += ' ';
			line += dur;
		}
	}
	return line;
}

int parseIntDefault(const std::string& s, int defVal) {
	if (s == "-" || s.empty()) {
		return defVal;
	}
	try { return std::stoi(s); } catch (...) { return defVal; }
}

std::unique_ptr<SkillCard> makeSkillCardFromSpec(const std::string& name, const std::string& v1, const std::string& v2) {
	if (name == "MoveCard" || name == "MOVE")
		return std::make_unique<MoveCard>(std::max(1, parseIntDefault(v1, 3)));
	if (name == "DiscountCard" || name == "DISCOUNT")
		return std::make_unique<DiscountCard>(parseIntDefault(v1, 30), std::max(1, parseIntDefault(v2, 1)));
	if (name == "ShieldCard" || name == "SHIELD")
		return std::make_unique<ShieldCard>(std::max(1, parseIntDefault(v1, 1)));
	if (name == "TeleportCard" || name == "TELEPORT") return std::make_unique<TeleportCard>();
	if (name == "LassoCard" || name == "LASSO") return std::make_unique<LassoCard>();
	if (name == "DemolitionCard" || name == "DEMOLITION") return std::make_unique<DemolitionCard>();
	return nullptr;
}

std::unique_ptr<SkillCard> makeDefaultSkillCard(const std::string& name) {
	if (name == "MoveCard") return std::make_unique<MoveCard>(3);
	if (name == "DiscountCard") return std::make_unique<DiscountCard>(30, 1);
	if (name == "ShieldCard") return std::make_unique<ShieldCard>(1);
	if (name == "TeleportCard") return std::make_unique<TeleportCard>();
	if (name == "LassoCard") return std::make_unique<LassoCard>();
	if (name == "DemolitionCard") return std::make_unique<DemolitionCard>();
	return nullptr;
}

int timesAppliedFromFmult(int fmult) {
	if (fmult >= 8) return 3;
	if (fmult >= 4) return 2;
	if (fmult >= 2) return 1;
	return 0;
}

void resetBoardProperties(Board& board) {
	for (int i = 0; i < board.getSize(); ++i) {
		Tile* tile = board.getTile(i);
		auto* prop = dynamic_cast<PropertyTile*>(tile);
		if (!prop) continue;
		prop->setOwner(nullptr);
		prop->setStatus(PropertyStatus::BANK);
		if (auto* street = dynamic_cast<StreetTile*>(prop)) {
			while (street->getBuildingLevel() > 0) {
				street->demolish();
			}
		}
	}
}

}  // namespace

bool TextFileRepository::exists(const std::string& id) const {
	std::ifstream in(id);
	return static_cast<bool>(in);
}

std::vector<std::string> TextFileRepository::getPlayerNames(const std::string& id) {
	std::ifstream in(id);
	if (!in) {
		return {};
	}
	std::string turnLine;
	if (!std::getline(in, turnLine)) {
		return {};
	}
	std::string nPlayersLine;
	if (!std::getline(in, nPlayersLine)) {
		return {};
	}
	int nPlayers = 0;
	{
		std::istringstream ps(nPlayersLine);
		if (!(ps >> nPlayers)) {
			return {};
		}
	}
	std::vector<std::string> names;
	names.reserve(static_cast<std::size_t>(nPlayers));
	for (int i = 0; i < nPlayers; ++i) {
		std::string statusLine;
		if (!std::getline(in, statusLine)) {
			break;
		}
		std::string username;
		{
			std::istringstream ls(statusLine);
			if (!(ls >> username)) {
				break;
			}
		}
		names.push_back(username);
		std::string cardCountLine;
		if (!std::getline(in, cardCountLine)) {
			break;
		}
		int nCards = 0;
		{
			std::istringstream cs(cardCountLine);
			cs >> nCards;
		}
		for (int c = 0; c < nCards; ++c) {
			std::string cardLine;
			std::getline(in, cardLine);
		}
	}
	return names;
}

bool TextFileRepository::save(const GameState& state, const Board& board, const TransactionLogger& logger,
	const FestivalManager& festivals, const CardSystem& cardSystem, const std::string& id) {
	std::ofstream out(id);
	if (!out) {
		return false;
	}

	const auto& players = state.getPlayers();
	const int nPlayers = static_cast<int>(players.size());

	out << state.getCurrentTurn() << ' ' << state.getMaxTurn() << '\n';
	out << nPlayers << '\n';

	std::map<std::string, std::pair<int, int>> festivalByCode;
	for (const FestivalEffectSnapshot& snap : festivals.getActiveEffectsSnapshot()) {
		if (snap.getProperty()) {
			festivalByCode[snap.getProperty()->getCode()] = {snap.getMultiplier(), snap.getTurnsRemaining()};
		}
	}

	for (Player* p : players) {
		if (!p) continue;
		Tile* tile = board.getTile(p->getPosition());
		const std::string tileCode = tile ? tile->getCode() : "GO";
		out << p->getUsername() << ' ' << p->getMoney().getAmount() << ' ' << tileCode << ' '
			<< playerStatusToken(p->getStatus()) << ' ' << p->getTurnCount() << ' ' << p->getJailTurnsRemaining() << '\n';
		const auto& cards = p->getSkillCards();
		out << static_cast<int>(cards.size()) << '\n';
		for (SkillCard* c : cards) {
			if (!c) continue;
			out << cardSaveLine(c) << '\n';
		}
	}

	const auto& order = state.getTurnOrder();
	for (std::size_t i = 0; i < order.size(); ++i) {
		if (i) out << ' ';
		out << (order[i] ? order[i]->getUsername() : "");
	}
	out << '\n';

	Player* active = state.getActivePlayer();
	out << (active ? active->getUsername() : "") << '\n';

	int propCount = 0;
	for (int i = 0; i < board.getSize(); ++i) {
		if (dynamic_cast<const PropertyTile*>(board.getTile(i))) {
			++propCount;
		}
	}
	out << propCount << '\n';

	for (int i = 0; i < board.getSize(); ++i) {
		Tile* tile = board.getTile(i);
		auto* prop = dynamic_cast<PropertyTile*>(tile);
		if (!prop) continue;

		int fmult = 1;
		int fdur = 0;
		const auto it = festivalByCode.find(prop->getCode());
		if (it != festivalByCode.end()) {
			fmult = it->second.first;
			fdur = it->second.second;
		}

		std::string ownerName = prop->getOwner() ? prop->getOwner()->getUsername() : "BANK";

		std::string buildStr = "0";
		if (auto* street = dynamic_cast<StreetTile*>(prop)) {
			buildStr = street->hasHotel() ? "H" : std::to_string(street->getBuildingLevel());
		}

		out << prop->getCode() << ' ' << propertyKindLower(*prop) << ' ' << ownerName << ' '
			<< propertyStatusToken(prop->getStatus()) << ' ' << fmult << ' ' << fdur << ' ' << buildStr << '\n';
	}

	const auto& skillDraw = cardSystem.getSkillDeck().getDrawCards();
	out << static_cast<int>(skillDraw.size()) << '\n';
	for (const auto& c : skillDraw) {
		if (c) out << c->getId() << '\n';
	}

	const std::vector<LogEntry> log = logger.serializeForSave();
	out << static_cast<int>(log.size()) << '\n';
	for (const LogEntry& e : log) {
		out << e.turn << ' ' << e.username << ' ' << e.actionType << ' ' << e.detail << '\n';
	}

	return static_cast<bool>(out);
}

bool TextFileRepository::loadInto(GameState& state, Board& board, TransactionLogger& logger, FestivalManager& festivals,
	CardSystem& cardSystem, const std::string& id) {
	std::ifstream in(id);
	if (!in) {
		return false;
	}

	resetBoardProperties(board);
	festivals.clearAllEffects();

	std::map<std::string, Player*> byName;
	for (Player* p : state.getPlayers()) {
		if (p) {
			p->clearSkillCardsAndEffects();
			p->resetTurnFlags();
			p->setConsecutiveDoubles(0);
			byName[p->getUsername()] = p;
		}
	}

	std::string turnLine;
	if (!std::getline(in, turnLine)) {
		return false;
	}
	int currentTurn = 0;
	int maxTurn = 0;
	{
		std::istringstream hs(turnLine);
		if (!(hs >> currentTurn >> maxTurn)) {
			return false;
		}
	}

	std::string nPlayersLine;
	if (!std::getline(in, nPlayersLine)) {
		return false;
	}
	int nPlayers = 0;
	{
		std::istringstream ps(nPlayersLine);
		if (!(ps >> nPlayers)) {
			return false;
		}
	}

	for (int i = 0; i < nPlayers; ++i) {
		std::string statusLine;
		if (!std::getline(in, statusLine)) {
			return false;
		}
		std::string username;
		int money = 0;
		std::string tileCode;
		std::string statusTok;
		int savedTurnCount = 0;
		int savedJailTurns = 0;
		{
			std::istringstream ls(statusLine);
			if (!(ls >> username >> money >> tileCode >> statusTok)) {
				return false;
			}
			ls >> savedTurnCount >> savedJailTurns;
		}

		std::string cardCountLine;
		if (!std::getline(in, cardCountLine)) {
			return false;
		}
		int nCards = 0;
		{
			std::istringstream cs(cardCountLine);
			cs >> nCards;
		}

		Player* player = byName.count(username) ? byName[username] : nullptr;
		if (player) {
			player->deductMoney(player->getMoney());
			player->addMoney(Money(money));
			player->setStatus(parsePlayerStatus(statusTok));
			Tile* at = board.getTileByCode(toUpper(tileCode));
			player->setPosition(at ? at->getId() : 0);
			player->setTurnCount(savedTurnCount);
			player->setJailTurnsRemaining(savedJailTurns);
		}

		for (int c = 0; c < nCards; ++c) {
			std::string cardLine;
			if (!std::getline(in, cardLine)) {
				break;
			}
			if (!player) {
				continue;
			}
			std::istringstream cls(cardLine);
			std::string cardType;
			std::string v1;
			std::string v2;
			if (!(cls >> cardType)) {
				continue;
			}
			cls >> v1 >> v2;
			auto card = makeSkillCardFromSpec(cardType, v1, v2);
			if (card) {
				try {
					player->addSkillCard(card.release());
				} catch (...) {
				}
			}
		}
	}

	std::string orderLine;
	if (!std::getline(in, orderLine)) {
		return false;
	}
	std::istringstream os(orderLine);
	std::vector<Player*> newOrder;
	std::string nameTok;
	while (os >> nameTok) {
		const auto it = byName.find(nameTok);
		if (it != byName.end() && it->second) {
			newOrder.push_back(it->second);
		}
	}
	if (newOrder.empty()) {
		newOrder.assign(state.getPlayers().begin(), state.getPlayers().end());
	}
	state.setTurnOrder(newOrder);

	std::string activeLine;
	if (!std::getline(in, activeLine)) {
		return false;
	}
	{
		std::istringstream as(activeLine);
		std::string activeName;
		as >> activeName;
		int activeIndex = 0;
		for (std::size_t i = 0; i < newOrder.size(); ++i) {
			if (newOrder[i] && newOrder[i]->getUsername() == activeName) {
				activeIndex = static_cast<int>(i);
				break;
			}
		}
		state.setActivePlayerIndex(activeIndex);
	}
	state.setCurrentTurn(currentTurn);
	state.setMaxTurn(maxTurn);
	state.setPhase(GamePhase::RUNNING);

	int nProps = 0;
	std::string propCountLine;
	if (!std::getline(in, propCountLine)) {
		return false;
	}
	{
		std::istringstream ps(propCountLine);
		if (!(ps >> nProps)) {
			return false;
		}
	}

	for (int p = 0; p < nProps; ++p) {
		std::string pline;
		if (!std::getline(in, pline)) {
			return false;
		}
		std::istringstream pls(pline);
		std::string code;
		std::string kind;
		std::string ownerName;
		std::string pst;
		int fmult = 1;
		int fdur = 0;
		std::string buildStr;
		if (!(pls >> code >> kind >> ownerName >> pst >> fmult >> fdur >> buildStr)) {
			continue;
		}
		int buildLevel = 0;
		if (buildStr == "H") {
			buildLevel = 5;
		} else if (!buildStr.empty() && buildStr != "-") {
			try { buildLevel = std::stoi(buildStr); } catch (...) { buildLevel = 0; }
		}

		PropertyTile* prop = board.getPropertyByCode(toUpper(code));
		if (!prop) {
			continue;
		}

		Player* owner = nullptr;
		if (ownerName != "BANK" && ownerName != "-") {
			const auto it = byName.find(ownerName);
			if (it != byName.end()) {
				owner = it->second;
			}
		}
		prop->setOwner(owner);
		prop->setStatus(parsePropertyStatus(pst));
		if (owner) {
			owner->addProperty(prop);
		}

		if (auto* street = dynamic_cast<StreetTile*>(prop)) {
			while (street->getBuildingLevel() > 0) {
				street->demolish();
			}
			for (int b = 0; b < buildLevel; ++b) {
				street->build();
			}
		}

		if (fmult > 1 && fdur > 0 && owner) {
			festivals.restoreEffect(prop, owner, fmult, fdur, timesAppliedFromFmult(fmult));
		}
	}

	board.updateMonopolies();

	std::string deckCountLine;
	if (!std::getline(in, deckCountLine)) {
		logger.clear();
		return true;
	}
	int nDeckCards = 0;
	{
		std::istringstream ds(deckCountLine);
		if (!(ds >> nDeckCards)) {
			logger.clear();
			return true;
		}
	}
	cardSystem.getSkillDeck().clear();
	for (int i = 0; i < nDeckCards; ++i) {
		std::string cardName;
		if (!std::getline(in, cardName)) {
			break;
		}
		while (!cardName.empty() && (cardName.back() == '\r' || cardName.back() == ' ')) {
			cardName.pop_back();
		}
		auto card = makeDefaultSkillCard(cardName);
		if (card) {
			cardSystem.getSkillDeck().addCard(std::move(card));
		}
	}

	std::string logCountLine;
	if (!std::getline(in, logCountLine)) {
		logger.clear();
		return true;
	}
	int nLog = 0;
	{
		std::istringstream ls(logCountLine);
		if (!(ls >> nLog)) {
			logger.clear();
			return true;
		}
	}

	std::vector<LogEntry> loaded;
	loaded.reserve(static_cast<std::size_t>(nLog));
	for (int i = 0; i < nLog; ++i) {
		std::string logLine;
		if (!std::getline(in, logLine)) {
			break;
		}
		std::istringstream ls(logLine);
		LogEntry e;
		std::string turnStr;
		if (!(ls >> turnStr >> e.username >> e.actionType)) {
			continue;
		}
		try { e.turn = std::stoi(turnStr); } catch (...) { continue; }
		std::getline(ls, e.detail);
		if (!e.detail.empty() && e.detail.front() == ' ') {
			e.detail = e.detail.substr(1);
		}
		loaded.push_back(std::move(e));
	}
	logger.loadFromSave(loaded);
	return true;
}
