#include "core/command/header/CommandRegistry.hpp"

void CommandRegistry::registerCommand(const Command& command) {
	commands[command.getName()] = command;
}

bool CommandRegistry::hasCommand(const std::string& name) const {
	return commands.find(name) != commands.end();
}

bool CommandRegistry::execute(const std::string& name) const {
	auto it = commands.find(name);
	if (it == commands.end()) {
		return false;
	}
	it->second.execute();
	return true;
}
