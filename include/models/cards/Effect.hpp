#pragma once

#include <string>

#include "models/Money.hpp"

class PropertyTile;

class Effect {
    public:
        explicit Effect(int remainingTurns);
        virtual ~Effect();

        virtual void onStart();
        virtual void onEnd();
        virtual Money modifyOutgoingPayment(const Money& amount) const;
        virtual Money modifyRent(const Money& amount, const PropertyTile* property) const;
        virtual bool blockPayment() const;

        bool isExpired() const;
        int getRemainingTurns() const;
        void setRemainingTurns(int turns);

        virtual std::string getDescription() const = 0;
        virtual std::string getEffectType() const = 0;

    private:
        int remainingTurns;
};

class ShieldEffect : public Effect {
    public:
        explicit ShieldEffect(int remainingTurns = 1);

        bool blockPayment() const override;
        std::string getDescription() const override;
        std::string getEffectType() const override;
};

class DiscountEffect : public Effect {
    public:
        DiscountEffect(int percentage, int remainingTurns = 1);

        Money modifyOutgoingPayment(const Money& amount) const override;
        int getPercentage() const;
        std::string getDescription() const override;
        std::string getEffectType() const override;

    private:
        int percentage;
};
