#include "models/cards/CardResult.hpp"

#include <utility>

CardResult CardResult::ok(std::string message) {
    CardResult result;
    result.message = std::move(message);
    return result;
}

CardResult CardResult::withAction(CardResultAction action, std::string message) {
    CardResult result = CardResult::ok(std::move(message));
    result.action = action;
    return result;
}

CardResult CardResult::fail(std::string message) {
    CardResult result;
    result.success = false;
    result.message = std::move(message);
    result.consumesCard = false;
    return result;
}
