#ifndef CORE_TEXT_FILE_REPOSITORY_HPP
#define CORE_TEXT_FILE_REPOSITORY_HPP

#include "core/IGameRepository.hpp"

class TextFileRepository : public IGameRepository {
public:
	bool save(const GameState& state, const Board& board, const TransactionLogger& logger, const FestivalManager& festivals, const std::string& id) override;
	bool loadInto(GameState& state, Board& board, TransactionLogger& logger, FestivalManager& festivals, const std::string& id) override;
	std::vector<std::string> getPlayerNames(const std::string& id) override;
	bool exists(const std::string& id) const override;
};

#endif
