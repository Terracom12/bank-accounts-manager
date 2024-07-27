#include "savings_account.h"
#include "util/date_util.h"

SavingsAccount::SavingsAccount(const SavingsAccount& other)
    : AccountInfo(other)
    , interestHandler_{other.interestHandler_}
    , timeManager_(other.timeManager_.clone([this](DatePeriod period) { update(period); })) {}

void SavingsAccount::deposit(Money amount) {
    balance_ += amount;

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = "Successful deposit",
                                                      .balanceChange = amount,
                                                      .changeType = StatementRecordInfo::Increase,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void SavingsAccount::withdraw(Money amount) {
    std::string statementInfo{};

    bool canWithdraw = amount <= balance_;

    if (canWithdraw) {
        balance_ += amount;
        statementInfo = fmt::format("Successful withdrawal");
    } else {
        statementInfo = fmt::format("Failed to withdraw {}", amount);
    }

    addToMonthlyStatement(timeManager_.getDate(), {
                                                      .details = statementInfo,
                                                      .balanceChange = canWithdraw ? amount : 0_dollars,
                                                      .changeType = StatementRecordInfo::Decrease,
                                                      .resultantBalance = getBalance(),
                                                  });
}

void SavingsAccount::update(DatePeriod period) {
    addStatementsThrough(period.end);
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
