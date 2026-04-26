#ifndef UTILS_GAME_UTILS_HPP
#define UTILS_GAME_UTILS_HPP

#include <string>
#include <vector>
#include "utils/Enums.hpp"

std::string lower(std::string value);
std::string normalizeCode(std::string value);
std::string displayName(std::string value);
std::string colorName(Color color);
std::string tileTypeName(TileType type);
std::vector<std::string> splitTokens(const std::string& line);
bool parseIntToken(const std::string& token, int& out);
Color parseColor(const std::string& raw);
bool fileExists(const std::string& path);

#endif
