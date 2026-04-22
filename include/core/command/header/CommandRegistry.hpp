#ifndef CORE_COMMAND_HEADER_COMMAND_REGISTRY_HPP
#define CORE_COMMAND_HEADER_COMMAND_REGISTRY_HPP

#include <string>
#include <unordered_map>

#include "core/command/header/Command.hpp"

class CommandRegistry {
public:
	void registerCommand(const Command& command);
	bool hasCommand(const std::string& name) const;
	bool execute(const std::string& name) const;

private:
	std::unordered_map<std::string, Command> commands;

protected:
};

#endif
