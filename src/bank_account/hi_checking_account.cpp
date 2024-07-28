#include "hi_checking_account.h"
#include "money_type.h"
#include "util/date_util.h"

// TODO: Try to reduce code duplication

HighInterestCheckingAccount::HighInterestCheckingAccount(const HighInterestCheckingAccount& other)
    : AccountInfo(other)
    , interestHandler_{other.interestHandler_}
    , timeManager_(other.timeManager_.clone([this](DatePeriod period) { update(period); })) {}

HighInterestCheckingAccount& HighInterestCheckingAccount::operator=(const HighInterestCheckingAccount& rhs) {
    if (&rhs == this) {
        return *this;
    }

    HighInterestCheckingAccount copy{rhs};
    *this = std::move(copy);
    return *this;
}

void HighInterestCheckingAccount::deposit(Money amount) {
    balance_ += amount;

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = "Successful deposit",
                                                      .balanceChange = amount,
                                                      .changeType = StatementRecordInfo::Increase,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void HighInterestCheckingAccount::writeCheck(Money amount) {
    withdrawHelper(amount, "check");
}

void HighInterestCheckingAccount::withdraw(Money amount) {
    withdrawHelper(amount, "withdrawal");
}

void HighInterestCheckingAccount::withdrawHelper(Money amount, std::string_view action) {
    std::string statementInfo{};
    bool success{};

    if (balance_ <= MIN_BALANCE) {
        statementInfo = fmt::format("{} failure ({}). Account balance already at or below minimum of {}.", action,
                                    amount, MIN_BALANCE);
        success = false;
    } else if (amount <= balance_ - MIN_BALANCE) {
        statementInfo = fmt::format("Successful {}", action);
        balance_ -= amount;
        success = true;
    } else {
        statementInfo =
            fmt::format("{} failure ({}). Would place account below minimum of {}.", action, amount, MIN_BALANCE);
        success = false;
    }

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = statementInfo,
                                                      .balanceChange = success ? amount : 0_dollars,
                                                      .changeType = StatementRecordInfo::Decrease,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void HighInterestCheckingAccount::update(DatePeriod period) {
    addStatementsThrough(period.end);
    // Do not earn any interest if balance is below minumum
    if (balance_ < MIN_BALANCE) {
        addToMonthlyStatement(timeManager_.getDate(),
                              {
                                  .details = "Failed to earn interest as account balance is below minimum requirement",
                                  .balanceChange = 0_dollars,
                                  .changeType = StatementRecordInfo::Increase,
                                  .resultantBalance = getBalance(),
                              });
        return;
    }

    interestHandler_.processDuring(period.begin, period.end, [&](Date when, double rate) {
        auto interestAmount = getBalance() * rate * INTEREST_MULTIPLIER;
        balance_ += interestAmount;
        addToMonthlyStatement(when, {
                                        .details = fmt::format("Accumulated interest at {:.2f}%", rate * 100),
                                        .balanceChange = interestAmount,
                                        .changeType = StatementRecordInfo::Increase,
                                        .resultantBalance = getBalance(),
                                    });
    });
}
