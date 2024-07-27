#include "cd_account.h"
#include "account_info.h"
#include "bank_account.h"
#include "util/date_util.h"

CertificateOfDepositAccount::CertificateOfDepositAccount(const CertificateOfDepositAccount& other)
    : AccountInfo(other)
    , numMaturityMonths_{other.numMaturityMonths_}
    , interestHandler_{other.interestHandler_}
    , earlyWithdrawalPenalty_{other.earlyWithdrawalPenalty_}
    , lastInterestPayment_{other.lastInterestPayment_}
    , timeManager_(other.timeManager_.clone([this](DatePeriod period) { update(period); })) {}

void CertificateOfDepositAccount::update(DatePeriod period) {
    fmt::println("updated!!!");
    addStatementsThrough(period.end);
    if (isMature_) {
        return;
    }

    interestHandler_.processDuring(period.begin, period.end, [this](Date when, double rate) {
        if (isMature_) {
            return;
        }

        auto interestAmount = getBalance() * rate;
        balance_ -= interestAmount;
        addToMonthlyStatement(when, {
                                        .details = fmt::format("Accumulated interest at {:.2f}%", rate * 100),
                                        .balanceChange = interestAmount,
                                        .changeType = StatementRecordInfo::Increase,
                                        .resultantBalance = getBalance(),
                                    });

        if (Date::diff<std::chrono::months>(getAccountOpeningDate(), when) >= numMaturityMonths_) {
            isMature_ = true;
            addToMonthlyStatement(Date{getAccountOpeningDate().get() + std::chrono::months{numMaturityMonths_}},
                                  {
                                      .details = "CD Account fully matured",
                                      .balanceChange = 0_dollars,
                                      .changeType = StatementRecordInfo::None,
                                      .resultantBalance = getBalance(),
                                  });
        }
    });
}

void CertificateOfDepositAccount::deposit(Money /*amount*/) {}
void CertificateOfDepositAccount::withdraw(Money amount) {
    std::string statementInfo{};

    auto penalty = amount * earlyWithdrawalPenalty_;
    auto fullAmount = isMature_ ? amount : amount + penalty;

    bool canWithdraw = fullAmount <= getBalance();

    if (canWithdraw && isMature_) {
        balance_ -= fullAmount;
        statementInfo = fmt::format("Successful withdrawal");
    } else if (canWithdraw) {
        balance_ -= fullAmount;
        statementInfo = fmt::format("Successful withdrawal of {} with penalty of {}", amount, penalty);
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
