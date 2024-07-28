#pragma once

#include "util/util.h"

#include <algorithm>
#include <compare>
#include <cstdint>
#include <fmt/format.h>
#include <locale>
#include <stdexcept>

// Only supports USD for now
class Money
{
public:
    constexpr Money() = default;
    constexpr Money(int dollars, int cents) // NOLINT(*easily-swappable-parameters)
        : dollars_{static_cast<uint64_t>(dollars)}
        , cents_{static_cast<uint64_t>(cents)} {
        handleChange();
        // TODO
        util::ctassert(dollars >= 0, "Dollars must be positive");
        util::ctassert(cents >= 0, "Cents must be positive");
    }

    constexpr std::uint64_t dollars() const { return dollars_; }
    constexpr std::uint64_t cents() const { return cents_; }

    explicit operator std::string() const {
        // TODO: Better currency support; or just force en_US format
        return fmt::format(std::locale{}, "${:L}.{:02}", dollars_, cents_);
    }

    constexpr Money operator+(const Money& rhs) const { return Money{*this} += rhs; }
    constexpr Money& operator+=(const Money& rhs) {
        dollars_ += rhs.dollars_;
        cents_ += rhs.cents_;
        handleChange();
        return *this;
    }
    constexpr Money operator-(Money rhs) const { return Money{*this} -= rhs; }
    constexpr Money& operator-=(const Money& rhs) {
        dollars_ -= rhs.dollars_;
        cents_ -= rhs.cents_;
        handleChange();
        return *this;
    }

    constexpr Money operator*(Money rhs) const { return Money{*this} *= rhs; }
    constexpr Money& operator*=(const Money& rhs) {
        dollars_ *= rhs.dollars_;
        cents_ *= rhs.cents_;
        handleChange();
        return *this;
    }
    constexpr Money operator*(double rhs) const { return Money{*this} *= rhs; }
    constexpr Money& operator*=(double rhs) {
        dollars_ = static_cast<std::uint64_t>(static_cast<double>(dollars_) * rhs);
        cents_ = static_cast<std::uint64_t>(static_cast<double>(cents_) * rhs);
        handleChange();
        return *this;
    }

    std::strong_ordering operator<=>(const Money& rhs) const = default;
    bool operator==(const Money& rhs) const = default;

private:
    constexpr void handleChange() {
        if (cents_ < 100) {
            return;
        }

        dollars_ += cents_ / 100;
        cents_ %= 100;
    }

    std::uint64_t dollars_ = 0;
    std::uint64_t cents_ = 0;
};

// TODO: Refactor
constexpr Money operator""_dollars(const char* str) {
    // NOLINTBEGIN
    auto dollars = util::atoi(str);

    const char* decPoint = std::find(str, str + util::strlen(str), '.');

    if (decPoint == nullptr || util::strlen(decPoint + 1) == 0) {
        return Money{dollars, 0};
    }

    auto cents = util::atoi(decPoint + 1);

    if (util::strlen(decPoint + 1) > 2) {
        throw std::runtime_error("More than 2 decimal places");
    }

    return Money{dollars, cents};
    // NOLINTEND
}
// NOLINTNEXTLINE(google-runtime-int)
constexpr Money operator""_dollars(unsigned long long amount) {
    return {static_cast<int>(amount), 0};
}
// NOLINTNEXTLINE(google-runtime-int)
constexpr Money operator""_cents(unsigned long long amount) {
    if (amount >= 100) {
        throw std::range_error("Cents must be < 99!");
    }

    return {0, static_cast<int>(amount)};
}

template <>
struct fmt::formatter<Money> : formatter<std::string>
{
    template <class FmtContext>
    FmtContext::iterator format(Money amt, FmtContext& ctx) const {
        return fmt::formatter<std::string>::format(std::string{amt}, ctx);
    }
};
