#pragma once

#include "util.h"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <chrono>
#include <compare>
#include <functional>
#include <limits>
#include <map>
#include <type_traits>
#include <utility>

inline std::chrono::year_month_day today() {
    const auto now = std::chrono::system_clock::now();

    return std::chrono::floor<std::chrono::days>(now);
}
inline std::chrono::year_month_day getMonthStart(std::chrono::year_month_day date) {
    auto prevMonth = date - std::chrono::months{1};
    auto prevMonthLastDay = std::chrono::year_month_day_last{prevMonth.year(), prevMonth.month() / std::chrono::last};
    return std::chrono::sys_days{prevMonthLastDay} + std::chrono::days{1};
}
inline std::chrono::year_month_day getMonthEnd(std::chrono::year_month_day date) {
    auto monthEnd = std::chrono::year_month_day_last{date.year(), date.month() / std::chrono::last};
    return monthEnd;
}

/**
 * @brief Basic gregorian date to simplify working with std::chrono.
 *
 */
class Date final
{
public:
    /**
     * @brief Construct a new Date object with the current date.
     *
     */
    Date()
        : date_{} {}

    explicit Date(std::chrono::year_month_day date)
        : date_{std::chrono::sys_days{date}} {}
    // explicit Date(int day, int month, int year)
    //     : Date(std::chrono::year_month_day{std::chrono::year{year},
    //                                        std::chrono::month{static_cast<unsigned int>(month)},
    //                                        std::chrono::day{static_cast<unsigned int>(day)}}) {}

    std::chrono::year_month_day get() const { return date_; }

    template <typename Ret = std::chrono::year_month_day>
    Ret to() const {
        return std::chrono::floor<Ret>(date_);
    }

    template <typename DurationType = std::chrono::days>
        requires std::convertible_to<DurationType, std::chrono::seconds>
    static constexpr std::int64_t diff(const Date& first, const Date& last) {
        const auto sysFirst = std::chrono::sys_days{first.date_};
        const auto sysLast = std::chrono::sys_days{last.date_};

        const auto sysDiff = sysLast - sysFirst;
        return std::chrono::floor<DurationType>(sysDiff).count();
    }

    bool operator==(const Date& other) const = default;
    std::strong_ordering operator<=>(const Date& other) const = default;

    friend struct fmt::formatter<Date>;

private:
    std::chrono::year_month_day date_;
};

// Inclusive period between two dates
struct DatePeriod
{
    Date begin;
    Date end;
};

template <>
struct fmt::formatter<Date> : formatter<std::chrono::sys_days>
{
    format_context::iterator format(Date date, format_context& ctx) const {
        return fmt::formatter<std::chrono::sys_days>::format(std::chrono::sys_days{date.date_}, ctx);
    }
};

namespace detail {

template <typename>
class Registry
{
public:
    using CallbackType = std::function<void(DatePeriod)>;

    static int registerFn(const CallbackType& func) {
        int id = makeUniqueId();
        [[maybe_unused]] auto result = callbacks_.insert({id, func});
        util::ctassert(result.second, "Attempting to register with multiple of the same id");
        return id;
    }

    static void deregisterFn(int id) {
        auto result = callbacks_.erase(id);
        util::ctassert(result != 0, "Failed to erase registered function");
    }

    static void updateAll(const std::function<void(int, const CallbackType&)>& func) {
        for (const auto& [id, callback] : callbacks_) {
            func(id, callback);
        }
    }

private:
    static int makeUniqueId() {
        auto iter = std::max_element(callbacks_.begin(), callbacks_.end(),
                                     [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
        if (iter == callbacks_.end()) {
            return 0;
        }

        int val = iter->first;

        util::ctassert(val < std::numeric_limits<int>::max(), "Id's are out of range of int");

        return val + 1;
    }

    inline static std::map<int, CallbackType> callbacks_; // NOLINT
};

} // namespace detail

class RealTimeManager : public detail::Registry<RealTimeManager>
{
public:
    static void updateAll() {
        auto date = getDate();
        // clang-format off
        detail::Registry<RealTimeManager>::updateAll([&](int /*id*/, auto& callback) {
            callback(DatePeriod{lastUpdate_, date});
        });
        // clang-format on

        lastUpdate_ = date;
    }

    static Date getDate() {
        const auto now = std::chrono::system_clock::now();

        return Date{std::chrono::floor<std::chrono::days>(now)};
    }

    friend class TimeManagerResource;

private:
    inline static Date lastUpdate_{getDate()}; // NOLINT
};

class SimTimeManager : public detail::Registry<SimTimeManager>
{
public:
    static void updateAll() {
        // clang-format off
        detail::Registry<SimTimeManager>::updateAll([](int /*id*/, auto& callback) {
            callback(DatePeriod{lastUpdate_, date_});
        });
        // clang-format on

        lastUpdate_ = date_;
    }

    static Date getDate() { return date_; }

    static void incrDay(std::chrono::days amt = std::chrono::days{1}) {
        date_ = Date{std::chrono::sys_days{date_.get()} + amt};
    }

    static void resetDay() {
        date_ = Date{::today()};
        lastUpdate_ = date_;
    }

    friend class TimeManagerResource;

private:
    inline static Date date_{::today()};   // NOLINT
    inline static Date lastUpdate_{date_}; // NOLINT
};

/**
 * @brief Concept for a TimeManager to be used with bank account systems.
 *
 * Semantic requirement: Time moves in one direction--forwards. In other words,
 * each subsequent call to @ref getDay() must return a @ref Date the same as or
 * past the point in time returned by the previous calls.
 */
template <typename T>
concept TimeManager = std::is_empty_v<T> && requires {
    requires std::same_as<typename T::CallbackType, std::function<void(DatePeriod)>>;
    {
        T::registerFn([](DatePeriod) -> void {})
    } -> std::same_as<int>;
    { T::deregisterFn(100) };
    { T::updateAll() };
    { T::getDate() } -> std::same_as<Date>;
};

static_assert(TimeManager<RealTimeManager>);
static_assert(TimeManager<SimTimeManager>);

class TimeManagerResource
{
public:
    using CallbackType = std::function<void(DatePeriod)>;

    template <TimeManager TimeMngT>
    TimeManagerResource(TimeMngT /*manager*/, CallbackType updateCallback)
        : getDateFn_{[] { return TimeMngT::getDate(); }}
        , registerFn_{TimeMngT::registerFn}
        , deregisterFn_{TimeMngT::deregisterFn}
        , callbackFn_{std::move(updateCallback)} {
        registerHelper();
    }

    TimeManagerResource(TimeManagerResource&& other) = default;
    TimeManagerResource& operator=(const TimeManagerResource&) = delete;
    TimeManagerResource& operator=(TimeManagerResource&& rhs) noexcept {
        if (id_ >= 0) {
            deregisterFn_(id_);
            id_ = -1;
        }

        swap(*this, rhs);
        return *this;
    }
    ~TimeManagerResource() {
        if (id_ >= 0) {
            deregisterFn_(id_);
            id_ = -1;
        }
    }

    TimeManagerResource clone(const CallbackType& newCallback) const {
        TimeManagerResource copy{*this};

        copy.callbackFn_ = newCallback;
        copy.registerHelper();

        return copy;
    }

    Date getDate() { return getDateFn_(); }

    friend void swap(TimeManagerResource& first, TimeManagerResource& second) noexcept {
        using std::swap;

        swap(first.id_, second.id_);
        swap(first.registerFn_, second.registerFn_);
        swap(first.deregisterFn_, second.deregisterFn_);
        swap(first.callbackFn_, second.callbackFn_);
    }

private:
    // Copying is only allowed as a helper internally for `clone`, since it could refer
    // to a class member function which is going out of scope.
    TimeManagerResource(const TimeManagerResource& other)
        : getDateFn_{other.getDateFn_}
        , registerFn_{other.registerFn_}
        , deregisterFn_{other.deregisterFn_}
        , callbackFn_{other.callbackFn_} {}

    void registerHelper() {
        id_ = registerFn_([&](DatePeriod period) { callbackFn_(period); });
    }

    int id_{};
    std::function<Date()> getDateFn_;
    std::function<int(const CallbackType&)> registerFn_;
    std::function<void(int)> deregisterFn_;
    CallbackType callbackFn_;
};
