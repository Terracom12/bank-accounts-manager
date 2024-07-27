#pragma once

#include "account_info.h"
#include "bank_account.h"
#include "interest_handler.h"
#include "util/date_util.h"

class CertificateOfDepositAccount : public AccountInfo
{
public:
    /**
     * @brief Construct a new Certificate Of Deposit Account object.
     *
     * @tparam TimeMngT Type to manage time functions like interest and records.
     * @param holderName Full name of the account holder
     * @param startingBalance Starting balance of the account
     * @param numMaturityMonths Number of months the CD takes to mature.
     * @param earlyWithdrawalPenalty Early withdrawal penalty as a proportion less than 1.0
     * @param interestHandler Information relating to how interest is handled
     * @param timeManager
     */
    CertificateOfDepositAccount(std::string_view holderName, Money startingBalance,
                                std::chrono::months numMaturityMonths, double earlyWithdrawalPenalty,
                                const InterestHandler& interestHandler, TimeManager auto timeManager = {});
    CertificateOfDepositAccount(const CertificateOfDepositAccount& other);

    /**
     * @brief Deposits on a CD are disallowed. Any call to this function is a no-op.
     *
     * @param amount Unused
     */
    void deposit(Money /*amount*/);
    void withdraw(Money amount);

protected:
    void update(DatePeriod period);

private:
    int numMaturityMonths_;
    bool isMature_{false};
    InterestHandler interestHandler_;
    double earlyWithdrawalPenalty_;
    Date lastInterestPayment_;
    TimeManagerResource timeManager_;
};

CertificateOfDepositAccount::CertificateOfDepositAccount(std::string_view holderName, Money startingBalance,
                                                         std::chrono::months numMaturityMonths,
                                                         double earlyWithdrawalPenalty,
                                                         const InterestHandler& interestHandler,
                                                         TimeManager auto timeManager)
    : AccountInfo(holderName, startingBalance, timeManager.getDate())
    , numMaturityMonths_{static_cast<int>(numMaturityMonths.count())}
    , interestHandler_{interestHandler}
    , earlyWithdrawalPenalty_{earlyWithdrawalPenalty}
    , lastInterestPayment_{getAccountOpeningDate()}
    , timeManager_(timeManager, [this](DatePeriod period) { update(period); }) {
    addToMonthlyStatement(timeManager.getDate(), {.details = "Account opened",
                                                  .balanceChange = getBalance(),
                                                  .changeType = StatementRecordInfo::None,
                                                  .resultantBalance = getBalance()});
}

static_assert(BankAccountConcept<CertificateOfDepositAccount>);
