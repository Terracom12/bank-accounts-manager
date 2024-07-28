#include "account_info.h"
#include "monthly_statement.h"

#include <cstddef>

AccountInfo::AccountInfo(std::string_view holderName, Money startingBalance, Date openingDate)
    : balance_(startingBalance)
    , holderName_(holderName)
    , number_(generateNextAccountNum())
    , openingDate_(openingDate) {}

const MonthlyStatement& AccountInfo::getMonthlyStatement(Date when) const {
    return getStatementHelper(*this, when);
}

void AccountInfo::addStatementsThrough(Date when) {
    auto monthsSinceOpening =
        static_cast<std::size_t>(Date::diff<std::chrono::months>(Date{getMonthStart(openingDate_.get())}, when));
    auto originalSize = monthlyStatements_.size();

    int numNew = static_cast<int>(monthsSinceOpening - originalSize + 1);

    if (numNew <= 0) {
        return;
    }

    if (originalSize > 0) {
        monthlyStatements_.back().complete = true;
    }

    for (int i = 0; i < numNew; ++i) {
        std::chrono::year_month_day startDate{};
        if (monthlyStatements_.empty()) {
            startDate = getMonthStart(openingDate_.get());
        } else {
            startDate = monthlyStatements_.back().start.get() + std::chrono::months{1};
        }

        monthlyStatements_.emplace_back(
            // NOLINTNEXTLINE: A little more readable this way
            MonthlyStatement{
                .start = Date{startDate}, .end = Date{getMonthEnd(startDate)}, .records = {}, .complete = true});
    }

    monthlyStatements_.back().complete = false;
}
void AccountInfo::addToMonthlyStatement(Date when, StatementRecordInfo info) {
    addStatementsThrough(when);
    getStatementHelper(*this, when).records.emplace(when, std::move(info));
}

int AccountInfo::generateNextAccountNum() {
    static int number = 0;

    return number++;
}
