#ifndef CORE_CONFIG_HEADER_GAME_CONFIG_HPP
#define CORE_CONFIG_HEADER_GAME_CONFIG_HPP

class GameConfig {
public:
	GameConfig();

	int getPlayerCount() const;
	int getMaxTurns() const;
	int getStartingMoney() const;
	int getPphFlat() const;
	int getPphPercentage() const;
	int getPbmFlat() const;
	int getGoSalary() const;
	int getJailFine() const;

	void setPlayerCount(int value);
	void setMaxTurns(int value);
	void setStartingMoney(int value);
	void setPphFlat(int value);
	void setPphPercentage(int value);
	void setPbmFlat(int value);
	void setGoSalary(int value);
	void setJailFine(int value);
	void setTaxConfig(int pphFlatValue, int pphPercentageValue, int pbmFlatValue);
	void setSpecialConfig(int goSalaryValue, int jailFineValue);
	bool loadFromDirectory(const char* directory);

private:
	int playerCount;
	int maxTurns;
	int startingMoney;
	int pphFlat;
	int pphPercentage;
	int pbmFlat;
	int goSalary;
	int jailFine;

protected:
};

#endif
