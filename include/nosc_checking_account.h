#pragma once

#include "account_info.h"
#include "bank_account.h"
#include "interest_handler.h"
#include "money_type.h"
#include "util/date_util.h"

#include <string_view>

class NoServiceChargeCheckingAccount : public AccountInfo
{
public:
    NoServiceChargeCheckingAccount(std::string_view holderName, Money startingBalance,
                                   const InterestHandler& interestHandler, TimeManager auto timeManager = {});
    NoServiceChargeCheckingAccount(const NoServiceChargeCheckingAccount& other);

    void deposit(Money amount);
    void withdraw(Money amount);
    void writeCheck(Money amount);

private:
    void update(DatePeriod period);
    void withdrawHelper(Money amount, std::string_view action);

    constexpr static Money MIN_BALANCE = 100_dollars;

    InterestHandler interestHandler_;
    TimeManagerResource timeManager_;
};

NoServiceChargeCheckingAccount::NoServiceChargeCheckingAccount(std::string_view holderName, Money startingBalance,
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

static_assert(BankAccountConcept<NoServiceChargeCheckingAccount>);