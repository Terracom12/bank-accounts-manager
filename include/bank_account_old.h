#pragma once

#include "money_type.h"
#include "monthly_statement.h"
#include "util/date_util.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief A generalized type for any bank account. ADT with balance and basic holder information.
 *
 * bankAccount: Every bank account has an account number, the name of the holder,
 * and a balance. Therefore, instance such as name, accountNumber, and balance
 * should be declared in the abstract class bankAccount. Some operations common to
 * all types of accounts are retrieve account name, number, and balance; make deposits,
 * withdraw money, and create monthly statements. So include functions to implement
 * these operations. Some of these functions will be pure virtual.
 */
class BankAccount
{
public:
    template <TimeManager TimeMngT = RealTimeManager>
    BankAccount(std::string_view holderName, Money startingBalance, TimeMngT timeManager = {});

    std::string_view getAccountName() const;
    int getAccountNumber() const;
    Date getAccountOpeningDate() const;
    /**
     * @brief Returns the current balance of the account. Always positive; deficits have not been implemented.
     *
     * @return Money
     */
    Money getBalance() const;
    const MonthlyStatement& getMonthlyStatement(Date when) const;
    const std::vector<MonthlyStatement>& getAllMonthlyStatements() const;

    virtual void deposit(Money amount) = 0;
    /**
     * @brief Subtracts amount from the account's current balance.
     *
     * Sufficient balance should be checked for, as negative `Money` may invoke UB.
     *
     * @param amount The amount of money to withdraw from the account. Should be less than the return of `getBalance()`.
     */
    virtual void withdraw(Money amount) = 0;

    /**
     * @brief Ensures that all aspects of the account are up-to-date according to temporal
     * values obtained from the TimeMngT specified in the constructor.
     *
     * This should be called in any member function that needs to take an action in the present.
     * (i.e. withdrawals, deposits, records, interest)
     *
     * Should not be overriden, as an injection point is provided in @ref temporalUpdate(Date)
     * for derived classes to implement their own functionality.
     */
    void update();

    virtual ~BankAccount() = default;

protected:
    void addToMonthlyStatement(StatementRecordInfo info);
    void addToMonthlyStatement(Date when, StatementRecordInfo info);

    /**
     * @brief A time update injection point that base classes may optionally override
     * if they need to provide time-based functionality, like retroactively
     *
     * Guaranteed to be called in @ref update().
     *
     * @param date The date to which all records and actions should update.
     */
    virtual void temporalUpdate(Date lastUpdate, Date date);

private:
    // TODO: Could use type-erasure for this
    std::function<Date()> timeManagerFunc_;
    Date lastUpdate_;

    Money balance_;
    std::string holderName_;
    int accountNum_;
    Date accountOpeningDate_;
    std::vector<MonthlyStatement> monthlyStatements_;

    /**
     * @brief Updates the monthly statements to ensure they exist through the current date.
     *
     */
    void updateStatementsHelper(Date until);

    // Declared as static function to ensure thread safety, just in case this is ever made multithreaded
    static int getCurrentAccountNum();

    MonthlyStatement& getStatementHelper(Date when);
};

template <TimeManager TimeMngT>
BankAccount::BankAccount(std::string_view holderName, Money startingBalance, [[maybe_unused]] TimeMngT timeManager)
    : timeManagerFunc_{TimeMngT::getDay}
    , balance_{startingBalance}
    , holderName_{holderName}
    , accountNum_{getCurrentAccountNum()}
    , accountOpeningDate_{timeManagerFunc_()} {
    update();
    addToMonthlyStatement(accountOpeningDate_, {.details = "Account opened",
                                                .balanceChange = balance_,
                                                .changeType = StatementRecordInfo::None,
                                                .resultantBalance = balance_});
}