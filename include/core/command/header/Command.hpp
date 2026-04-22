#ifndef CORE_COMMAND_HEADER_COMMAND_HPP
#define CORE_COMMAND_HEADER_COMMAND_HPP

#include <functional>
#include <string>

class Command {
public:
	using Handler = std::function<void()>;

	Command(std::string name = "", Handler handler = nullptr);

	const std::string& getName() const;
	bool isValid() const;
	void execute() const;

private:
	std::string name;
	Handler handler;

protected:
};

#endif