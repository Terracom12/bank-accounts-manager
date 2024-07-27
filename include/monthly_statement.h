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
    format_context::iterator format(const StatementRecordInfo& s, format_context& ctx) const {
        const std::string balanceChange = fmt::format("{}{}", static_cast<char>(s.changeType), s.balanceChange);
        return fmt::format_to(ctx.out(), "{0:<80} [{1:>20}] ({2:>20})", s.details, balanceChange, s.resultantBalance);
    }
};

template <>
struct fmt::formatter<MonthlyStatement> : formatter<std::string>
{
    format_context::iterator format(const MonthlyStatement& s, format_context& ctx) const {
        auto iter = fmt::format_to(ctx.out(), "Monthly Statement for {0:%B} {0:%Y} ({0:%D} to {1:%D}){2}\n", s.start,
                                   s.end, s.complete ? "" : " INCOMPLETE");
        for (const auto& [date, rec] : s.records) {
            iter = fmt::format_to(ctx.out(), "\t{0:%m}/{0:%d}: {1}\n", date, rec);
        }

        if (s.records.empty()) {
            iter = fmt::format_to(ctx.out(), "\tNo records\n");
        }

        return iter;
    }
};