#include "hi_savings_account.h"
#include "util/date_util.h"

#include <fmt/core.h>

// TODO: Try to reduce duplicated code

HighInterestSavingsAccount::HighInterestSavingsAccount(const HighInterestSavingsAccount& other)
    : AccountInfo(other)
    , interestHandler_{other.interestHandler_}
    , timeManager_(other.timeManager_.clone([this](DatePeriod period) { update(period); })) {}

HighInterestSavingsAccount& HighInterestSavingsAccount::operator=(const HighInterestSavingsAccount& rhs) {
    if (&rhs == this) {
        return *this;
    }

    HighInterestSavingsAccount copy{rhs};
    *this = std::move(copy);
    return *this;
}

void HighInterestSavingsAccount::deposit(Money amount) {
    balance_ += amount;

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = "Successful deposit",
                                                      .balanceChange = amount,
                                                      .changeType = StatementRecordInfo::Increase,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void HighInterestSavingsAccount::withdraw(Money amount) {
    std::string statementInfo{};
    bool success{};

    if (balance_ <= MIN_BALANCE) {
        statementInfo = fmt::format("Failed to withdraw {}. Account balance already at or below minimum of {}.", amount,
                                    MIN_BALANCE);
        success = false;
    } else if (amount <= balance_ - MIN_BALANCE) {
        statementInfo = fmt::format("Successful withdrawal");
        balance_ -= amount;
        success = true;
    } else {
        statementInfo =
            fmt::format("Failed to withdraw {}. Would place account below minimum of {}.", amount, MIN_BALANCE);
        success = false;
    }

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = statementInfo,
                                                      .balanceChange = success ? amount : 0_dollars,
                                                      .changeType = StatementRecordInfo::Decrease,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void HighInterestSavingsAccount::update(DatePeriod period) {
    addStatementsThrough(period.end);
    // Do not earn any interest if balance is below minumum
    if (balance_ < MIN_BALANCE) {
        return;
    }

    // It would be much cleaner to just reuse `SavingsAccount::update` here
    interestHandler_.processDuring(period.begin, period.end, [&](Date when, double rate) {
        auto interestAmount = getBalance() * rate;
        balance_ += interestAmount;
        addToMonthlyStatement(when, {
                                        .details = fmt::format("Accumulated interest at {:.2f}%", rate * 100),
                                        .balanceChange = interestAmount,
                                        .changeType = StatementRecordInfo::Increase,
                                        .resultantBalance = getBalance(),
                                    });
    });
}
