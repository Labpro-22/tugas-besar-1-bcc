#include "models/cards/Card.hpp"

#include <utility>

Card::Card(std::string name, std::string description)
    : name(std::move(name)),
      description(std::move(description)) {}

Card::~Card() = default;

const std::string& Card::getName() const {
    return name;
}

const std::string& Card::getDescription() const {
    return description;
}
