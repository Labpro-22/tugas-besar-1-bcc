#include "core/config/header/GameConfig.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace {
	int tableValue(const std::vector<int>& table, int count) {
		if (count <= 0 || table.empty()) {
			return 0;
		}

		if (count < static_cast<int>(table.size()) && table[static_cast<std::size_t>(count)] > 0) {
			return table[static_cast<std::size_t>(count)];
		}

		for (int i = static_cast<int>(table.size()) - 1; i >= 1; --i) {
			if (table[static_cast<std::size_t>(i)] > 0) {
				return table[static_cast<std::size_t>(i)];
			}
		}
		return 0;
	}

	std::vector<int> readIndexedTable(const std::string& path) {
		std::ifstream file(path);
		std::vector<int> table(1, 0);
		std::string headerA;
		std::string headerB;
		if (!(file >> headerA >> headerB)) {
			return {};
		}

		int index = 0;
		int value = 0;
		while (file >> index >> value) {
			if (index < 0) {
				continue;
			}
			if (index >= static_cast<int>(table.size())) {
				table.resize(static_cast<std::size_t>(index) + 1, 0);
			}
			table[static_cast<std::size_t>(index)] = value;
		}

		return table.size() > 1 ? table : std::vector<int>{};
	}
}

GameConfig::GameConfig()
	: playerCount(4), maxTurns(100), startingMoney(1000000), pphFlat(50000), pphPercentage(10), pbmFlat(50000),
		  goSalary(200000), jailFine(50000), railroadRentTable{0, 25, 50, 100, 200},
		  utilityMultiplierTable{0, 4, 10} {}

int GameConfig::getPlayerCount() const {
	return playerCount;
}

int GameConfig::getMaxTurns() const {
	return maxTurns;
}

int GameConfig::getStartingMoney() const {
	return startingMoney;
}

int GameConfig::getPphFlat() const {
	return pphFlat;
}

int GameConfig::getPphPercentage() const {
	return pphPercentage;
}

int GameConfig::getPbmFlat() const {
	return pbmFlat;
}

int GameConfig::getGoSalary() const {
	return goSalary;
}

int GameConfig::getJailFine() const {
	return jailFine;
}

int GameConfig::getRailroadRent(int ownedCount) const {
	return tableValue(railroadRentTable, ownedCount);
}

int GameConfig::getUtilityMultiplier(int ownedCount) const {
	return tableValue(utilityMultiplierTable, ownedCount);
}

const std::vector<int>& GameConfig::getRailroadRentTable() const {
	return railroadRentTable;
}

const std::vector<int>& GameConfig::getUtilityMultiplierTable() const {
	return utilityMultiplierTable;
}

void GameConfig::setPlayerCount(int value) {
	playerCount = std::max(0, value);
}

void GameConfig::setMaxTurns(int value) {
	maxTurns = std::max(0, value);
}

void GameConfig::setStartingMoney(int value) {
	startingMoney = std::max(0, value);
}

void GameConfig::setPphFlat(int value) {
	pphFlat = std::max(0, value);
}

void GameConfig::setPphPercentage(int value) {
	pphPercentage = std::max(0, value);
}

void GameConfig::setPbmFlat(int value) {
	pbmFlat = std::max(0, value);
}

void GameConfig::setGoSalary(int value) {
	goSalary = std::max(0, value);
}

void GameConfig::setJailFine(int value) {
	jailFine = std::max(0, value);
}

void GameConfig::setRailroadRentTable(const std::vector<int>& values) {
	if (!values.empty()) {
		railroadRentTable = values;
	}
}

void GameConfig::setUtilityMultiplierTable(const std::vector<int>& values) {
	if (!values.empty()) {
		utilityMultiplierTable = values;
	}
}

void GameConfig::setTaxConfig(int pphFlatValue, int pphPercentageValue, int pbmFlatValue) {
	setPphFlat(pphFlatValue);
	setPphPercentage(pphPercentageValue);
	setPbmFlat(pbmFlatValue);
}

void GameConfig::setSpecialConfig(int goSalaryValue, int jailFineValue) {
	setGoSalary(goSalaryValue);
	setJailFine(jailFineValue);
}

bool GameConfig::loadFromDirectory(const char* directory) {
	const std::string base = directory ? directory : "config";
	bool loadedAny = false;

	{
		std::ifstream file(base + "/misc.txt");
		std::string header;
		int maxTurnValue = 0;
		int startingMoneyValue = 0;
		if (file >> header) {
			std::string secondHeader;
			file >> secondHeader;
			if (file >> maxTurnValue >> startingMoneyValue) {
				setMaxTurns(maxTurnValue);
				setStartingMoney(startingMoneyValue);
				loadedAny = true;
			}
		}
	}

	{
		std::ifstream file(base + "/tax.txt");
		std::string h1, h2, h3;
		int pphFlatValue = 0;
		int pphPercentageValue = 0;
		int pbmFlatValue = 0;
		if (file >> h1 >> h2 >> h3 >> pphFlatValue >> pphPercentageValue >> pbmFlatValue) {
			setTaxConfig(pphFlatValue, pphPercentageValue, pbmFlatValue);
			loadedAny = true;
		}
	}

	{
		std::ifstream file(base + "/special.txt");
		std::string h1, h2;
		int goSalaryValue = 0;
		int jailFineValue = 0;
		if (file >> h1 >> h2 >> goSalaryValue >> jailFineValue) {
			setSpecialConfig(goSalaryValue, jailFineValue);
			loadedAny = true;
		}
	}

	{
		const std::vector<int> table = readIndexedTable(base + "/railroad.txt");
		if (!table.empty()) {
			setRailroadRentTable(table);
			loadedAny = true;
		}
	}

	{
		const std::vector<int> table = readIndexedTable(base + "/utility.txt");
		if (!table.empty()) {
			setUtilityMultiplierTable(table);
			loadedAny = true;
		}
	}

	return loadedAny;
}
