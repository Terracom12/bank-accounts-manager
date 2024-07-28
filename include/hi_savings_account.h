#pragma once

#include "account_info.h"
#include "bank_account.h"
#include "interest_handler.h"
#include "money_type.h"

// Due to the nature of type erasure as it's used on these classes,
// most of the code here corresponds one-to-one with `SavingsAccount`
// as previously was reused via inheritance.

class HighInterestSavingsAccount : public AccountInfo
{
public:
    HighInterestSavingsAccount(std::string_view holderName, Money startingBalance,
                               const InterestHandler& interestHandler, TimeManager auto timeManager = {});
    HighInterestSavingsAccount(const HighInterestSavingsAccount& other);
    HighInterestSavingsAccount(HighInterestSavingsAccount&&) = default;
    HighInterestSavingsAccount& operator=(const HighInterestSavingsAccount&);
    HighInterestSavingsAccount& operator=(HighInterestSavingsAccount&&) = default;
    ~HighInterestSavingsAccount() = default;

    void deposit(Money amount);
    void withdraw(Money amount);

private:
    void update(DatePeriod period);

    constexpr static double INTEREST_MULTIPLIER = 2.5;
    constexpr static Money MIN_BALANCE = 10'000_dollars;

    InterestHandler interestHandler_;
    TimeManagerResource timeManager_;
};

HighInterestSavingsAccount::HighInterestSavingsAccount(std::string_view holderName, Money startingBalance,
                                                       const InterestHandler& interestHandler,
                                                       TimeManager auto timeManager)
    : AccountInfo(holderName, startingBalance, timeManager.getDate())
    , interestHandler_(interestHandler)
    , timeManager_(timeManager, [this](DatePeriod period) { update(period); }) {
    addToMonthlyStatement(timeManager.getDate(), {.details = "Account opened",
                                                  .balanceChange = getBalance(),
                                                  .changeType = StatementRecordInfo::None,
                                                  .resultantBalance = getBalance()});
}

static_assert(BankAccountConcept<HighInterestSavingsAccount>);
