#ifndef MONEY_HPP
#define MONEY_HPP

#include <string>
using namespace std;

class Money {
    public:
        explicit Money(int amount = 0);

        int getAmount() const;
        void setAmount(int value);

        bool isZero() const;
        bool isPositive() const;

        Money operator+(const Money& other) const;
        Money operator-(const Money& other) const;

        Money& operator+=(const Money& other);
        Money& operator-=(const Money& other);

        bool operator<(const Money& other) const;
        bool operator>(const Money& other) const;
        bool operator<=(const Money& other) const;
        bool operator>=(const Money& other) const;
        bool operator==(const Money& other) const;
        bool operator!=(const Money& other) const;

        static Money zero();

        string toString() const;

    private:
        int amount;
};

#endif
