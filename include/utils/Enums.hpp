#pragma once

#include <cstdint>
#include <iostream>
using namespace std;

/**
 * @brief
 * PROJECT WIDE ENUMS DI SINI YA GES BIAR GA CIRCULAR INCLUDES
 */


enum class TileType : uint8_t {
    STREET,
    RAILROAD,
    UTILITY,
    CHANCE,
    COMMUNITY_CHEST,
    FESTIVAL,
    TAX_PPH,
    TAX_PBM,
    GO,
    JAIL,
    FREE_PARKING,
    GO_TO_JAIL,
};

enum class PropertyStatus : uint8_t {
    BANK,
    OWNED,
    MORTGAGED,
};

enum class PlayerStatus : uint8_t {
    ACTIVE,
    BANKRUPT,
    JAILED,
};

enum class Color : uint8_t {
    BROWN,
    LIGHT_BLUE,
    PINK,
    ORANGE,
    RED,
    YELLOW,
    GREEN,
    DARK_BLUE,
    DEFAULT,
    GRAY,
};

enum class SkillCardType : uint8_t {
    MOVE,
    DISCOUNT,
    SHIELD,
    TELEPORT,
    LASSO,
    DEMOLITION,
};

enum class LandingAction : uint8_t {
    BUY_PROMPT,
    PAY_RENT,
    PAY_TAX,
    DRAW_CHANCE,
    DRAW_COMMUNITY,
    FESTIVAL_PROMPT,
    GO_TO_JAIL,
    NOTHING,
    AUTO_ACQUIRE,
    BANKRUPT_CHECK,
};

enum class TaxChoice : uint8_t {
    FLAT,
    PERCENTAGE,
};

enum class AuctionAction : uint8_t {
    BID,
    PASS,
};

enum class LiquidationType : uint8_t {
    SELL,
    MORTGAGE,
};

enum class JailEntryReason : uint8_t {
    GO_TO_JAIL_TILE,
    TRIPLE_DOUBLE,
    CHANCE_CARD,
};

enum class AILevel : uint8_t {
    BASIC,
    SMART,
};

/**
 * @brief
 * one value per concrete GameEvent subclass
 * concrete values can be finalized when events are implemented
 */
enum class EventType : uint16_t {
    PLAYER_MOVED,
    MONEY_CHANGED,
    PROPERTY_ACQUIRED,
    RENT_PAID,
    BUILDING_BUILT,
    MORTGAGED,
    REDEEMED,
    FESTIVAL_ACTIVATED,
    AUCTION_STARTED,
    BID_PLACED,
    CARD_DRAWN,
    SKILL_CARD_USED,
    EFFECT_APPLIED,
    BANKRUPTCY,
    TURN_STARTED,
    TURN_ENDED,
    GAME_OVER,
};
