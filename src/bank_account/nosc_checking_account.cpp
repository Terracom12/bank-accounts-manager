#include "nosc_checking_account.h"
#include "money_type.h"
#include "util/date_util.h"

NoServiceChargeCheckingAccount::NoServiceChargeCheckingAccount(const NoServiceChargeCheckingAccount& other)
    : AccountInfo(other)
    , interestHandler_{other.interestHandler_}
    , timeManager_(other.timeManager_.clone([this](DatePeriod period) { update(period); })) {}

void NoServiceChargeCheckingAccount::deposit(Money amount) {
    balance_ += amount;

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = "Successful deposit",
                                                      .balanceChange = amount,
                                                      .changeType = StatementRecordInfo::Increase,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void NoServiceChargeCheckingAccount::writeCheck(Money amount) {
    withdrawHelper(amount, "check");
}

void NoServiceChargeCheckingAccount::withdraw(Money amount) {
    withdrawHelper(amount, "withdrawal");
}

void NoServiceChargeCheckingAccount::withdrawHelper(Money amount, std::string_view action) {
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

void NoServiceChargeCheckingAccount::update(DatePeriod period) {
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
