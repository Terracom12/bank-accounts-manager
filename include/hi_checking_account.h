#pragma once

#include "account_info.h"
#include "bank_account.h"
#include "interest_handler.h"
#include "util/date_util.h"

// Due to the nature of type erasure as it's used on these classes,
// most of the code here corresponds one-to-one with `NoServiceChargeCheckingAccount`
// as previously was reused via inheritance.

class HighInterestCheckingAccount : public AccountInfo
{
public:
    HighInterestCheckingAccount(std::string_view holderName, Money startingBalance,
                                const InterestHandler& interestHandler, TimeManager auto timeManager = {});
    HighInterestCheckingAccount(const HighInterestCheckingAccount& other);

    void deposit(Money amount);
    void withdraw(Money amount);
    void writeCheck(Money amount);

private:
    void update(DatePeriod period);
    void withdrawHelper(Money amount, std::string_view action);

    constexpr static Money MIN_BALANCE = 500_dollars;
    constexpr static double INTEREST_MULTIPLIER = 2.5;

    InterestHandler interestHandler_;
    TimeManagerResource timeManager_;
};

HighInterestCheckingAccount::HighInterestCheckingAccount(std::string_view holderName, Money startingBalance,
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

static_assert(BankAccountConcept<HighInterestCheckingAccount>);