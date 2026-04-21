#pragma once

#include <string>

class Card {
    public:
        Card(std::string name, std::string description);
        virtual ~Card();

        Card(const Card& other) = delete;
        Card& operator=(const Card& other) = delete;

        const std::string& getName() const;
        const std::string& getDescription() const;
        virtual std::string getId() const = 0;
        virtual std::string getType() const = 0;

    private:
        std::string name;
        std::string description;
};
