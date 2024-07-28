#pragma once

#include "account_info.h"
#include "bank_account.h"
#include "interest_handler.h"
#include "util/date_util.h"

class SavingsAccount : public AccountInfo
{
public:
    SavingsAccount(std::string_view holderName, Money startingBalance, const InterestHandler& interestHandler,
                   TimeManager auto timeManager = {});
    SavingsAccount(const SavingsAccount& other);
    SavingsAccount(SavingsAccount&&) = default;
    SavingsAccount& operator=(const SavingsAccount&);
    SavingsAccount& operator=(SavingsAccount&&) = default;
    ~SavingsAccount() = default;

    void deposit(Money amount);
    void withdraw(Money amount);

private:
    void update(DatePeriod period);

    InterestHandler interestHandler_;
    TimeManagerResource timeManager_;
};

SavingsAccount::SavingsAccount(std::string_view holderName, Money startingBalance,
                               const InterestHandler& interestHandler, TimeManager auto timeManager)
    : AccountInfo(holderName, startingBalance, timeManager.getDate())
    , interestHandler_(interestHandler)
    , timeManager_(timeManager, [this](DatePeriod period) { update(period); }) {
    addToMonthlyStatement(timeManager.getDate(), {.details = "Account opened",
                                                  .balanceChange = getBalance(),
                                                  .changeType = StatementRecordInfo::None,
                                                  .resultantBalance = getBalance()});
}

static_assert(BankAccountConcept<SavingsAccount>);
