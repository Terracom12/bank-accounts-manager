#include "interest_handler.h"

#include <chrono>
#include <ratio>

InterestHandler::InterestHandler(InterestType compoundingPeriod, double rate)
    : compoundingPeriod_{compoundingPeriod}
    , rate_{rate} {}

void InterestHandler::processDuring(Date begin, Date end, const std::function<void(Date, double)>& interestPayoutFn) {
    if (!lastPayment_.get().ok()) {
        lastPayment_ = begin;
    }

    for (std::chrono::sys_days day = begin.get(); day <= end.get(); day += std::chrono::days{1}) {
        tryProcessOn(Date{day}, interestPayoutFn);
    }
}

void InterestHandler::tryProcessOn(Date day, const std::function<void(Date, double)>& interestPayoutFn) {
    using enum InterestType;

    switch (compoundingPeriod_) {
    case Daily:
        if (Date::diff<std::chrono::days>(lastPayment_, day) > 0) {
            lastPayment_ = day;
            interestPayoutFn(day, rate_);
            break;
        }
        return;

    case Monthly:
        if (Date::diff<std::chrono::months>(lastPayment_, day) > 0) {
            lastPayment_ = day;
            interestPayoutFn(day, rate_);
            break;
        }
        return;

    case Quarterly:
        using QuarterlyT = std::chrono::duration<int, std::ratio_multiply<std::ratio<3>, std::chrono::months::period>>;
        if (Date::diff<QuarterlyT>(lastPayment_, day) > 0) {
            lastPayment_ = day;
            interestPayoutFn(day, rate_);
            break;
        }
        return;

    case Yearly:
        if (Date::diff<std::chrono::years>(lastPayment_, day) > 0) {
            lastPayment_ = day;
            interestPayoutFn(day, rate_);
            break;
        }
        return;
    }
}
