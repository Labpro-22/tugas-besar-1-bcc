#ifndef CORE_IGAME_REPOSITORY_HPP
#define CORE_IGAME_REPOSITORY_HPP

#include <string>
#include <vector>

class GameState;
class Board;
class TransactionLogger;
class FestivalManager;

class IGameRepository {
public:
	virtual ~IGameRepository() = default;

	virtual bool save(const GameState& state, const Board& board, const TransactionLogger& logger, const FestivalManager& festivals, const std::string& id) = 0;
	virtual bool loadInto(GameState& state, Board& board, TransactionLogger& logger, FestivalManager& festivals, const std::string& id) = 0;
	virtual std::vector<std::string> getPlayerNames(const std::string& id) = 0;
	virtual bool exists(const std::string& id) const = 0;
};

#endif
