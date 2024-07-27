#pragma once

#include "fmt/core.h"
#include "util/date_util.h"
#include <functional>
#include <string_view>

enum class InterestType { Daily, Monthly, Quarterly, Yearly };

// For simple compounding interest
class InterestHandler
{
public:
    InterestHandler(InterestType compoundingPeriod, double rate);

    void processDuring(Date begin, Date end, const std::function<void(Date, double)>& interestPayoutFn);

    double getRate() const { return rate_; }

private:
    void tryProcessOn(Date day, const std::function<void(Date, double)>& interestPayoutFn);

    InterestType compoundingPeriod_;
    double rate_;

    Date lastPayment_{};
};

template <>
struct fmt::formatter<InterestType> : formatter<std::string_view>
{
    template <class FmtContext>
    FmtContext::iterator format(InterestType s, FmtContext& ctx) const {
        using enum InterestType;

        std::string_view str{};

        switch (s) {
        case Daily:
            str = "Daily";
            break;
        case Monthly:
            str = "Monthly";
            break;
        case Quarterly:
            str = "Quarterly";
            break;
        case Yearly:
            str = "Yearly";
            break;
        }
        return formatter<std::string_view>::format(str, ctx);
    }
};