#include "core/command/header/Command.hpp"

Command::Command(std::string name, Handler handler) : name(std::move(name)), handler(std::move(handler)) {}

const std::string& Command::getName() const {
	return name;
}

bool Command::isValid() const {
	return static_cast<bool>(handler);
}

void Command::execute() const {
	if (handler) {
		handler();
	}
}