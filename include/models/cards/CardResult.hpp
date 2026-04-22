#pragma once

#include <cstdint>
#include <string>

#include "models/Money.hpp"
#include "utils/Enums.hpp"

class Player;
class PropertyTile;

enum class CardResultAction : uint8_t {
    NONE,
    MOVE_RELATIVE,
    MOVE_TO_NEAREST_STATION,
    SEND_TO_JAIL,
    RECEIVE_FROM_EACH_PLAYER,
    PAY_BANK,
    PAY_EACH_PLAYER,
    APPLY_DISCOUNT,
    APPLY_SHIELD,
    TELEPORT,
    LASSO,
    DEMOLISH_PROPERTY,
};

class CardResult {
public:
    bool success{true};
    std::string message;
    CardResultAction action{CardResultAction::NONE};
    LandingAction nextAction{LandingAction::NOTHING};
    Money amount{0};
    Player* targetPlayer{nullptr};
    PropertyTile* targetProperty{nullptr};
    int value{0};
    int movementOffset{0};
    int percentage{0};
    int duration{0};
    int destinationIndex{-1};
    bool movedPlayer{false};
    bool resolveLanding{false};
    bool requiresTargetSelection{false};
    bool consumesCard{true};

    static CardResult ok(std::string message = "");
    static CardResult withAction(CardResultAction action, std::string message = "");
    static CardResult fail(std::string message);
};
