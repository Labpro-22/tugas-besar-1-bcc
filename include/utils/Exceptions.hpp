#pragma once

#include <exception>
#include <string>

#include "models/Money.hpp"

class NimonopoliException : public std::exception {
public:
    explicit NimonopoliException(std::string message);

    const char* what() const noexcept override;

private:
    std::string message;
};

class InsufficientFundsException final : public NimonopoliException {
public:
    InsufficientFundsException(Money required, Money available);

    Money getRequired() const;
    Money getAvailable() const;

private:
    Money required;
    Money available;
};

class InvalidCommandException final : public NimonopoliException {
public:
    explicit InvalidCommandException(std::string message);
};

class InvalidPropertyException final : public NimonopoliException {
public:
    explicit InvalidPropertyException(std::string message);
};

class CardSlotFullException final : public NimonopoliException {
public:
    explicit CardSlotFullException(int maxSlots);

    int getMaxSlots() const;

private:
    int maxSlots;
};

class InvalidBidException final : public NimonopoliException {
public:
    InvalidBidException(int bidAmount, std::string reason);

    int getBidAmount() const;
    const std::string& getReason() const;

private:
    int bidAmount;
    std::string reason;
};

class SaveLoadException final : public NimonopoliException {
public:
    explicit SaveLoadException(std::string message);
};

class InvalidBoardConfigException final : public NimonopoliException {
public:
    explicit InvalidBoardConfigException(std::string message);
};
