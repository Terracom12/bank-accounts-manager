#include "sc_checking_account.h"
#include "util/date_util.h"
#include <chrono>

ServiceChargeCheckingAccount::ServiceChargeCheckingAccount(const ServiceChargeCheckingAccount& other)
    : AccountInfo(other)
    , timeManager_(other.timeManager_.clone([this](DatePeriod period) { update(period); })) {}

void ServiceChargeCheckingAccount::deposit(Money amt) {
    balance_ += amt;

    const std::string statementInfo = fmt::format("Successful deposit", amt);
    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = statementInfo,
                                                      .balanceChange = amt,
                                                      .changeType = StatementRecordInfo::Increase,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void ServiceChargeCheckingAccount::withdraw(Money amt) {
    std::string statementInfo{};

    bool canWithdraw = amt <= getBalance();

    if (canWithdraw) {
        balance_ -= amt;
        statementInfo = fmt::format("Successful withdrawal");
    } else {
        statementInfo = fmt::format("Failed to withdraw {}", amt);
    }

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = statementInfo,
                                                      .balanceChange = canWithdraw ? amt : 0_dollars,
                                                      .changeType = StatementRecordInfo::Decrease,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void ServiceChargeCheckingAccount::writeCheck(Money amount) {
    if (remainingChecks_ <= 0) {
        addToMonthlyStatement(
            timeManager_.getDate(),
            {
                .details =
                    fmt::format("Failed to write check for {}. Reached maximum allowable checks in one month.", amount),
                .balanceChange = 0_dollars,
                .changeType = StatementRecordInfo::Decrease,
                .resultantBalance = getBalance(),
            });
        return;
    }

    remainingChecks_--;

    withdraw(amount);
}

void ServiceChargeCheckingAccount::update(DatePeriod period) {
    addStatementsThrough(period.end);
    if (!lastServiceCharge_.get().ok()) {
        lastServiceCharge_ = period.begin;
    }

    while (Date::diff<std::chrono::months>(lastServiceCharge_, period.end) >= 1) {
        auto dayToCharge = Date{lastServiceCharge_.get() + std::chrono::months{1}};
        lastServiceCharge_ = dayToCharge;
        remainingChecks_ = CHECKS_PER_MONTH;

        Money amount;
        std::string statementInfo;

        if (getBalance() >= SERVICE_CHARGE) {
            amount = SERVICE_CHARGE;
            statementInfo = "Service charge fee";
        } else if (getBalance() > 0_dollars) {
            statementInfo = "Service charge fee. Account has run out of funds.";
            amount = getBalance();
        } else {
            statementInfo = "Attempted service charge fee, but account is empty.";
            amount = 0_dollars;
        }

        balance_ -= amount;
        addToMonthlyStatement(dayToCharge, {
                                               .details = statementInfo,
                                               .balanceChange = amount,
                                               .changeType = StatementRecordInfo::Decrease,
                                               .resultantBalance = getBalance(),
                                           });
    }
}