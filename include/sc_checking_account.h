#pragma once

#include "account_info.h"
#include "bank_account.h"
#include "money_type.h"
#include "monthly_statement.h"
#include "util/date_util.h"

#include <string_view>

// TODO: Deregister from time manager

class ServiceChargeCheckingAccount : public AccountInfo
{
public:
    ServiceChargeCheckingAccount(std::string_view holderName, Money startingBalance, TimeManager auto timeManager = {});
    ServiceChargeCheckingAccount(const ServiceChargeCheckingAccount& other);

    void deposit(Money amt);
    void withdraw(Money amt);

    void writeCheck(Money amount);

    constexpr static Money SERVICE_CHARGE = 100_dollars;
    constexpr static int CHECKS_PER_MONTH = 10;

private:
    void update(DatePeriod period);
    void withdrawHelper(Money amount, std::string_view action);

    int remainingChecks_ = CHECKS_PER_MONTH;
    TimeManagerResource timeManager_;
    Date lastServiceCharge_;
};

ServiceChargeCheckingAccount::ServiceChargeCheckingAccount(std::string_view holderName, Money startingBalance,
                                                           TimeManager auto timeManager)
    : AccountInfo(holderName, startingBalance, timeManager.getDate())
    , timeManager_(timeManager, [this](DatePeriod period) { update(period); })
    , lastServiceCharge_(timeManager_.getDate()) {
    addToMonthlyStatement(timeManager.getDate(), {.details = "Account opened",
                                                  .balanceChange = getBalance(),
                                                  .changeType = StatementRecordInfo::None,
                                                  .resultantBalance = getBalance()});
}

static_assert(BankAccountConcept<ServiceChargeCheckingAccount>);
