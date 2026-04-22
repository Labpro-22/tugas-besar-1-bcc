#include "core/config/header/GameConfig.hpp"

#include <algorithm>
#include <fstream>
#include <string>

GameConfig::GameConfig()
	: playerCount(4), maxTurns(100), startingMoney(1000000), pphFlat(50000), pphPercentage(10), pbmFlat(50000),
	  goSalary(200000), jailFine(50000) {}

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

	return loadedAny;
}
