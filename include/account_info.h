/*! \file account_info.h
    \brief File containing the AccountInfo class and SelfT template

    The Account Info class is declared within this header file
*/
#pragma once

#include "money_type.h"
#include "monthly_statement.h"
#include "util/date_util.h"
#include "util/util.h"

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

//! Manages functions receiving basic information for BankAccount
/*!
  The AccountInfo class generates monthly statements and keep strack of the account information
*/
class AccountInfo
{
public:
    AccountInfo(std::string_view holderName, Money startingBalance, Date openingDate);

    std::string_view getAccountName() const { return holderName_; }
    int getAccountNumber() const { return number_; }
    Date getAccountOpeningDate() const { return openingDate_; }
    Money getBalance() const { return balance_; }
    const MonthlyStatement& getMonthlyStatement(Date when) const;
    const std::vector<MonthlyStatement>& getAllMonthlyStatements() const { return monthlyStatements_; }

protected:
    void addStatementsThrough(Date when);
    void addToMonthlyStatement(Date when, StatementRecordInfo info);

    Money balance_; // NOLINT

private:
    // Declared as static function to ensure thread safety
    static int generateNextAccountNum();

    template <typename SelfT>
        requires std::is_same_v<AccountInfo, std::remove_const_t<SelfT>>
    static util::retain_const_t<SelfT, MonthlyStatement&> getStatementHelper(SelfT& self, Date when);

    std::string holderName_;
    int number_;
    Date openingDate_;
    std::vector<MonthlyStatement> monthlyStatements_;
};

//! This template helps generate the monthly statements
template <typename SelfT>
    requires std::is_same_v<AccountInfo, std::remove_const_t<SelfT>>
util::retain_const_t<SelfT, MonthlyStatement&> AccountInfo::getStatementHelper(SelfT& self, Date when) {
    auto monthsSinceOpening = Date::diff<std::chrono::months>(Date{getMonthStart(self.openingDate_.get())}, when);

    return self.monthlyStatements_.at(static_cast<std::size_t>(monthsSinceOpening));
}
