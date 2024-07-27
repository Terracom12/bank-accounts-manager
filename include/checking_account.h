#pragma once

#include "bank_account.h"
#include "money_type.h"

// TODO: Type erase

class CheckingAccount
{
public:
    virtual ~CheckingAccount() = default;
    void deposit(Money amount);
    void withdraw(Money amount);
    virtual void writeCheck(Money amount) = 0;
};
