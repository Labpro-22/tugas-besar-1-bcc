#include "utils/GameUtils.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

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

bool fileExists(const std::string& path) {
	std::ifstream file(path);
	return static_cast<bool>(file);
}
