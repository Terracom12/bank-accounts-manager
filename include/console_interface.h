#pragma once

#include "bank_account.h"

#include <vector>

class ConsoleInterface
{
public:
    void run();

private:
    enum class MainMenuOption { NewAccount, StepDays, SelectAccount, Quit };
    enum class AccountMenuOption { Close, DisplayStatement, Info, Deposit, Withdraw, Back };

    MainMenuOption mainMenu() const;
    AccountMenuOption accountMenu(const BankAccount& account) const;

    void handleAccountAction(AccountMenuOption opt, BankAccount& account);
    BankAccount* selectAccount();
    void newAccount();

    std::vector<BankAccount> openAccounts_;
};