#pragma once

#include "fmt/core.h"
#include "money_type.h"
#include "util/date_util.h"
#include <fmt/format.h>
#include <map>
#include <string>

struct StatementRecordInfo
{
    std::string details;
    Money balanceChange;
    enum : char { Increase = '+', Decrease = '-', None = '=' } changeType;
    Money resultantBalance;
};

struct MonthlyStatement
{
    Date start;
    Date end;
    std::multimap<Date, StatementRecordInfo> records;
    bool complete = false;
};

template <>
struct fmt::formatter<StatementRecordInfo> : formatter<std::string>
{
    template <typename FormatCtx>
    FormatCtx::iterator format(const StatementRecordInfo& record, FormatCtx& ctx) const {
        std::string balanceChange = fmt::format("{}{}", static_cast<char>(record.changeType), record.balanceChange);
        std::string str =
            fmt::format("{0:<80} [{1:>20}] ({2:>20})", record.details, balanceChange, record.resultantBalance);
        return formatter<std::string>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<MonthlyStatement> : formatter<std::string>
{
    template <typename FormatCtx>
    FormatCtx::iterator format(const MonthlyStatement& statement, FormatCtx& ctx) const {
        auto iter = fmt::format_to(ctx.out(), "Monthly Statement for {0:%B} {0:%Y} ({0:%D} to {1:%D}){2}\n",
                                   statement.start, statement.end, statement.complete ? "" : " INCOMPLETE");
        for (const auto& [date, rec] : statement.records) {
            iter = fmt::format_to(ctx.out(), "\t{0:%m}/{0:%d}: {1}\n", date, rec);
        }

        if (statement.records.empty()) {
            iter = fmt::format_to(ctx.out(), "\tNo records\n");
        }

        return iter;
    }
};
