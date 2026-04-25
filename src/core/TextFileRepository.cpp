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

std::string propertyKind(const PropertyTile& p) {
	switch (p.getType()) {
		case TileType::STREET: return "STREET";
		case TileType::RAILROAD: return "RAILROAD";
		case TileType::UTILITY: return "UTILITY";
		default: return "OTHER";
	}
}

std::string skillTypeName(SkillCardType t) {
	switch (t) {
		case SkillCardType::MOVE: return "MOVE";
		case SkillCardType::DISCOUNT: return "DISCOUNT";
		case SkillCardType::SHIELD: return "SHIELD";
		case SkillCardType::TELEPORT: return "TELEPORT";
		case SkillCardType::LASSO: return "LASSO";
		case SkillCardType::DEMOLITION: return "DEMOLITION";
	}
	return "MOVE";
}

int parseIntDefault(const std::string& s, int defVal) {
	if (s == "-" || s.empty()) {
		return defVal;
	}
	return std::stoi(s);
}

std::unique_ptr<SkillCard> makeSkillCard(const std::string& type, const std::string& val, const std::string& dur) {
	if (type == "MOVE") {
		const int steps = std::max(1, parseIntDefault(val, 1));
		return std::make_unique<MoveCard>(steps);
	}
	if (type == "DISCOUNT") {
		return std::make_unique<DiscountCard>(parseIntDefault(val, 30), parseIntDefault(dur, 1));
	}
	if (type == "SHIELD") {
		return std::make_unique<ShieldCard>(std::max(1, parseIntDefault(dur, 1)));
	}
	if (type == "TELEPORT") return std::make_unique<TeleportCard>();
	if (type == "LASSO") return std::make_unique<LassoCard>();
	if (type == "DEMOLITION") return std::make_unique<DemolitionCard>();
	return nullptr;
}

std::unique_ptr<ChanceCard> makeChanceCard(const std::string& id) {
	if (id == "ChanceGoToNearestStationCard") return std::make_unique<ChanceGoToNearestStationCard>();
	if (id == "ChanceMoveBackThreeCard") return std::make_unique<ChanceMoveBackThreeCard>();
	if (id == "ChanceGoToJailCard") return std::make_unique<ChanceGoToJailCard>();
	return nullptr;
}

std::unique_ptr<CommunityChestCard> makeCommunityCard(const std::string& id) {
	if (id == "BirthdayCard") return std::make_unique<BirthdayCard>();
	if (id == "DoctorFeeCard") return std::make_unique<DoctorFeeCard>();
	if (id == "ElectionCampaignCard") return std::make_unique<ElectionCampaignCard>();
	return nullptr;
}

std::unique_ptr<SkillCard> makeSkillCardById(const std::string& id, const std::string& val, const std::string& dur) {
	if (id == "MoveCard") return std::make_unique<MoveCard>(std::max(1, parseIntDefault(val, 1)));
	if (id == "DiscountCard") return std::make_unique<DiscountCard>(parseIntDefault(val, 30), std::max(1, parseIntDefault(dur, 1)));
	if (id == "ShieldCard") return std::make_unique<ShieldCard>(std::max(1, parseIntDefault(dur, 1)));
	if (id == "TeleportCard") return std::make_unique<TeleportCard>();
	if (id == "LassoCard") return std::make_unique<LassoCard>();
	if (id == "DemolitionCard") return std::make_unique<DemolitionCard>();
	return nullptr;
}

std::vector<std::string> splitPipe(const std::string& line) {
	std::vector<std::string> parts;
	std::size_t start = 0;
	while (start <= line.size()) {
		const std::size_t pos = line.find('|', start);
		if (pos == std::string::npos) {
			parts.push_back(line.substr(start));
			break;
		}
		parts.push_back(line.substr(start, pos - start));
		start = pos + 1;
	}
	return parts;
}

int readTaggedCount(const std::string& line, const std::string& tag) {
	std::istringstream stream(line);
	std::string header;
	int count = 0;
	if (!(stream >> header >> count) || header != tag) {
		throw std::runtime_error("Malformed deck section: " + line);
	}
	return count;
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
	std::string header;
	std::getline(in, header);
	std::istringstream hs(header);
	int currentTurn = 0;
	int maxTurn = 0;
	int nPlayers = 0;
	if (!(hs >> currentTurn >> maxTurn >> nPlayers)) {
		return {};
	}
	std::vector<std::string> names;
	names.reserve(static_cast<std::size_t>(nPlayers));
	for (int i = 0; i < nPlayers; ++i) {
		std::string line;
		if (!std::getline(in, line)) {
			break;
		}
		std::istringstream ls(line);
		std::string username;
		if (!(ls >> username)) {
			break;
		}
		names.push_back(username);
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
	out << state.getCurrentTurn() << ' ' << state.getMaxTurn() << ' ' << nPlayers << ' ' << board.getSize() << '\n';

	std::map<std::string, std::tuple<int, int, int>> festivalByCode;
	for (const FestivalEffectSnapshot& snap : festivals.getActiveEffectsSnapshot()) {
		if (snap.getProperty()) {
			festivalByCode[snap.getProperty()->getCode()] =
				std::make_tuple(snap.getMultiplier(), snap.getTurnsRemaining(), snap.getTimesApplied());
		}
	}

	for (Player* p : players) {
		if (!p) continue;
		Tile* tile = board.getTile(p->getPosition());
		const std::string code = tile ? tile->getCode() : "GO";
		out << p->getUsername() << ' ' << p->getMoney().getAmount() << ' ' << code << ' '
			<< playerStatusToken(p->getStatus()) << ' ';
		const auto& cards = p->getSkillCards();
		out << cards.size();
		for (SkillCard* c : cards) {
			if (!c) continue;
			std::string v = c->getSaveValue();
			std::string d = c->getSaveDuration();
			out << ' ' << skillTypeName(c->getCardType()) << ' ' << (v.empty() ? "-" : v) << ' '
				<< (d.empty() ? "-" : d);
		}
		out << ' ' << p->getJailTurnsRemaining() << ' ' << p->getTurnCount();
		out << '\n';
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
		const Tile* tile = board.getTile(i);
		if (dynamic_cast<const PropertyTile*>(tile)) {
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
		int timesApplied = 0;
		const auto it = festivalByCode.find(prop->getCode());
		if (it != festivalByCode.end()) {
			fmult = std::get<0>(it->second);
			fdur = std::get<1>(it->second);
			timesApplied = std::get<2>(it->second);
		}

		std::string ownerName = "BANK";
		if (prop->getOwner()) {
			ownerName = prop->getOwner()->getUsername();
		}

		int buildLevel = 0;
		if (auto* street = dynamic_cast<StreetTile*>(prop)) {
			buildLevel = street->getBuildingLevel();
		}

		out << prop->getCode() << ' ' << propertyKind(*prop) << ' ' << ownerName << ' '
			<< propertyStatusToken(prop->getStatus()) << ' ' << fmult << ' ' << fdur << ' ' << buildLevel << ' '
			<< timesApplied << '\n';
	}

	const std::vector<LogEntry> log = logger.serializeForSave();
	out << static_cast<int>(log.size()) << '\n';
	for (const LogEntry& e : log) {
		out << e.turn << '|' << e.username << '|' << e.actionType << '|' << e.detail << '\n';
	}

	std::vector<std::tuple<std::string, std::string, int, int>> serializedEffects;
	for (Player* p : players) {
		if (!p) continue;
		for (Effect* effect : p->getActiveEffects()) {
			if (!effect) continue;
			int value = 0;
			if (auto* discount = dynamic_cast<DiscountEffect*>(effect)) {
				value = discount->getPercentage();
			}
			serializedEffects.emplace_back(
				p->getUsername(),
				toUpper(effect->getEffectType()),
				value,
				effect->getRemainingTurns());
		}
	}

	out << "<EFFECT_STATE>\n";
	out << static_cast<int>(serializedEffects.size()) << '\n';
	for (const auto& effect : serializedEffects) {
		out << std::get<0>(effect) << '|' << std::get<1>(effect) << '|' << std::get<2>(effect)
			<< '|' << std::get<3>(effect) << '\n';
	}

	out << "<DECK_STATE>\n";
	const auto writeSimpleDeck = [&](const std::string& tag, const auto& cards) {
		out << tag << ' ' << static_cast<int>(cards.size()) << '\n';
		for (const auto& card : cards) {
			out << (card ? card->getId() : "") << '\n';
		}
	};
	const auto writeSkillDeck = [&](const std::string& tag, const auto& cards) {
		out << tag << ' ' << static_cast<int>(cards.size()) << '\n';
		for (const auto& card : cards) {
			if (!card) {
				out << "||\n";
				continue;
			}
			out << card->getId() << '|' << (card->getSaveValue().empty() ? "-" : card->getSaveValue())
				<< '|' << (card->getSaveDuration().empty() ? "-" : card->getSaveDuration()) << '\n';
		}
	};

	writeSimpleDeck("CHANCE_DRAW", cardSystem.getChanceDeck().getDrawCards());
	writeSimpleDeck("CHANCE_USED", cardSystem.getChanceDeck().getUsedCards());
	writeSimpleDeck("COMMUNITY_DRAW", cardSystem.getCommunityChestDeck().getDrawCards());
	writeSimpleDeck("COMMUNITY_USED", cardSystem.getCommunityChestDeck().getUsedCards());
	writeSkillDeck("SKILL_DRAW", cardSystem.getSkillDeck().getDrawCards());
	writeSkillDeck("SKILL_USED", cardSystem.getSkillDeck().getUsedCards());

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
			p->setStatus(PlayerStatus::ACTIVE);
			p->setJailTurnsRemaining(0);
			p->setConsecutiveDoubles(0);
			p->setTurnCount(0);
			byName[p->getUsername()] = p;
		}
	}

	std::string header;
	std::getline(in, header);
	std::istringstream hs(header);
	int currentTurn = 0;
	int maxTurn = 0;
	int nPlayers = 0;
	if (!(hs >> currentTurn >> maxTurn >> nPlayers)) {
		return false;
	}

	for (int i = 0; i < nPlayers; ++i) {
		std::string line;
		if (!std::getline(in, line)) {
			return false;
		}
		std::istringstream ls(line);
		std::string username;
		int money = 0;
		std::string tileCode;
		std::string statusTok;
		int nCards = 0;
		int jailTurns = 0;
		int turnCount = 0;
		if (!(ls >> username >> money >> tileCode >> statusTok >> nCards)) {
			return false;
		}
		Player* player = nullptr;
		const auto it = byName.find(username);
		if (it != byName.end()) {
			player = it->second;
		}
		if (!player) {
			for (int c = 0; c < nCards; ++c) {
				std::string t, v, d;
				ls >> t >> v >> d;
			}
			continue;
		}

		player->deductMoney(player->getMoney());
		player->addMoney(Money(money));
		player->setStatus(parsePlayerStatus(statusTok));

		Tile* at = board.getTileByCode(toUpper(tileCode));
		if (at) {
			player->setPosition(at->getId());
		} else {
			player->setPosition(0);
		}

		for (int c = 0; c < nCards; ++c) {
			std::string typeTok;
			std::string valTok;
			std::string durTok;
			if (!(ls >> typeTok >> valTok >> durTok)) {
				break;
			}
			auto card = makeSkillCard(typeTok, valTok, durTok);
			if (card) {
				try {
					player->addSkillCard(card.release());
				} catch (...) {
				}
			}
		}
		if (!(ls >> jailTurns >> turnCount)) {
			jailTurns = 0;
			turnCount = 0;
		}
		player->setJailTurnsRemaining(jailTurns);
		player->setTurnCount(turnCount);
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
		int buildLevel = 0;
		int timesApplied = 0;
		if (!(pls >> code >> kind >> ownerName >> pst >> fmult >> fdur >> buildLevel >> timesApplied)) {
			if (!(pls >> code >> kind >> ownerName >> pst >> fmult >> fdur >> buildLevel)) {
				continue;
			}
			timesApplied = 1;
		}

		PropertyTile* prop = board.getPropertyByCode(toUpper(code));
		if (!prop) continue;

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
			festivals.restoreEffect(prop, owner, fmult, fdur, std::max(1, timesApplied));
		}
	}

	board.updateMonopolies();

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
		const std::size_t p1 = logLine.find('|');
		const std::size_t p2 = logLine.find('|', p1 == std::string::npos ? 0 : p1 + 1);
		const std::size_t p3 = logLine.find('|', p2 == std::string::npos ? 0 : p2 + 1);
		if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) {
			continue;
		}
		LogEntry e;
		e.turn = std::stoi(logLine.substr(0, p1));
		e.username = logLine.substr(p1 + 1, p2 - p1 - 1);
		e.actionType = logLine.substr(p2 + 1, p3 - p2 - 1);
		e.detail = logLine.substr(p3 + 1);
		loaded.push_back(std::move(e));
	}
	logger.loadFromSave(loaded);

	std::string marker;
	while (std::getline(in, marker)) {
		if (marker.empty()) {
			continue;
		}

		if (marker == "<EFFECT_STATE>") {
			std::string countLine;
			if (!std::getline(in, countLine)) {
				return true;
			}
			int effectCount = 0;
			{
				std::istringstream es(countLine);
				if (!(es >> effectCount)) {
					return true;
				}
			}
			for (int i = 0; i < effectCount; ++i) {
				std::string effectLine;
				if (!std::getline(in, effectLine)) {
					break;
				}
				const std::vector<std::string> parts = splitPipe(effectLine);
				if (parts.size() < 4) {
					continue;
				}
				const auto it = byName.find(parts[0]);
				if (it == byName.end() || !it->second) {
					continue;
				}
				Player* player = it->second;
				const std::string effectType = toUpper(parts[1]);
				const int value = parseIntDefault(parts[2], 0);
				const int turns = std::max(1, parseIntDefault(parts[3], 1));
				if (effectType == "DISCOUNT") {
					player->addEffect(new DiscountEffect(value, turns));
				} else if (effectType == "SHIELD") {
					player->addEffect(new ShieldEffect(turns));
				}
			}
			continue;
		}

		if (marker == "<DECK_STATE>") {
			cardSystem.getChanceDeck().clear();
			cardSystem.getCommunityChestDeck().clear();
			cardSystem.getSkillDeck().clear();

			std::vector<CardDeck<ChanceCard>::CardPtr> chanceDraw;
			std::vector<CardDeck<ChanceCard>::CardPtr> chanceUsed;
			std::vector<CardDeck<CommunityChestCard>::CardPtr> communityDraw;
			std::vector<CardDeck<CommunityChestCard>::CardPtr> communityUsed;
			std::vector<CardDeck<SkillCard>::CardPtr> skillDraw;
			std::vector<CardDeck<SkillCard>::CardPtr> skillUsed;

			try {
				std::string line;
				if (!std::getline(in, line)) break;
				const int chanceDrawCount = readTaggedCount(line, "CHANCE_DRAW");
				for (int i = 0; i < chanceDrawCount; ++i) {
					std::string idLine;
					if (!std::getline(in, idLine)) break;
					if (auto card = makeChanceCard(idLine)) {
						chanceDraw.push_back(std::move(card));
					}
				}

				if (!std::getline(in, line)) break;
				const int chanceUsedCount = readTaggedCount(line, "CHANCE_USED");
				for (int i = 0; i < chanceUsedCount; ++i) {
					std::string idLine;
					if (!std::getline(in, idLine)) break;
					if (auto card = makeChanceCard(idLine)) {
						chanceUsed.push_back(std::move(card));
					}
				}

				if (!std::getline(in, line)) break;
				const int communityDrawCount = readTaggedCount(line, "COMMUNITY_DRAW");
				for (int i = 0; i < communityDrawCount; ++i) {
					std::string idLine;
					if (!std::getline(in, idLine)) break;
					if (auto card = makeCommunityCard(idLine)) {
						communityDraw.push_back(std::move(card));
					}
				}

				if (!std::getline(in, line)) break;
				const int communityUsedCount = readTaggedCount(line, "COMMUNITY_USED");
				for (int i = 0; i < communityUsedCount; ++i) {
					std::string idLine;
					if (!std::getline(in, idLine)) break;
					if (auto card = makeCommunityCard(idLine)) {
						communityUsed.push_back(std::move(card));
					}
				}

				if (!std::getline(in, line)) break;
				const int skillDrawCount = readTaggedCount(line, "SKILL_DRAW");
				for (int i = 0; i < skillDrawCount; ++i) {
					std::string idLine;
					if (!std::getline(in, idLine)) break;
					const std::vector<std::string> parts = splitPipe(idLine);
					if (parts.size() < 3) continue;
					if (auto card = makeSkillCardById(parts[0], parts[1], parts[2])) {
						skillDraw.push_back(std::move(card));
					}
				}

				if (!std::getline(in, line)) break;
				const int skillUsedCount = readTaggedCount(line, "SKILL_USED");
				for (int i = 0; i < skillUsedCount; ++i) {
					std::string idLine;
					if (!std::getline(in, idLine)) break;
					const std::vector<std::string> parts = splitPipe(idLine);
					if (parts.size() < 3) continue;
					if (auto card = makeSkillCardById(parts[0], parts[1], parts[2])) {
						skillUsed.push_back(std::move(card));
					}
				}
			} catch (const std::exception&) {
				cardSystem.initializeDecks();
				continue;
			}

			cardSystem.getChanceDeck().setDrawCards(std::move(chanceDraw));
			cardSystem.getChanceDeck().setUsedCards(std::move(chanceUsed));
			cardSystem.getCommunityChestDeck().setDrawCards(std::move(communityDraw));
			cardSystem.getCommunityChestDeck().setUsedCards(std::move(communityUsed));
			cardSystem.getSkillDeck().setDrawCards(std::move(skillDraw));
			cardSystem.getSkillDeck().setUsedCards(std::move(skillUsed));
		}
	}

	return true;
}
