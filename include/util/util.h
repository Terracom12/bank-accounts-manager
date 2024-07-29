// A rewrite of a few stdlib functions to make them constexpr capable

#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

// NOLINTBEGIN(*pointer-arithmetic)

namespace util {

constexpr void ctassert(bool expr, const char* str) {
    if (!expr) {
        throw std::runtime_error(str);
    }
}

constexpr int strlen(const char* str) {
    int len = 0;

    while (*str != '\0') {
        ++str;
        ++len;
    }

    return len;
}

// NOLINTNEXTLINE(*identifier-naming)
constexpr bool is_space(char chr) {
    return chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t';
}

// NOLINTNEXTLINE(*identifier-naming)
constexpr bool is_digit(char chr) {
    return chr >= '0' && chr <= '9';
}

constexpr int atoi(const char* str) {
    while (is_space(*str)) {
        str++;
    }

    bool negative = false;
    std::int64_t val = 0;
    constexpr std::int64_t MIN = std::numeric_limits<int>::min();

    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        str++;
        negative = true;
    }

    ctassert(is_digit(*str), "Invalid character in parsed string");

    while (is_digit(*str)) {
        val *= 10;
        // Negative so that it won't overflow for min int value
        val -= (*str - '0');
        ctassert(val >= MIN, "atoi(const char*) call out of range for `int`");
        str++;
        // Skip ' as separator
        if (*str == '\'') {
            str++;
        }
    }

    ctassert(negative || val != MIN, "atoi(const char*) call out of range for `int`");

    int result = static_cast<int>(negative ? val : -val);

    return result;
}
// NOLINTEND(*pointer-arithmetic)

// NOLINTBEGIN(*identifier-naming)
template <typename Old, typename New>
struct retain_const
{
    using type = std::conditional_t<std::is_const_v<std::remove_reference_t<Old>>, const New, New>;
};
// Specialization if `New` is of reference type
template <typename Old, typename New>
struct retain_const<Old, New&>
{
    using type = std::conditional_t<std::is_const_v<std::remove_reference_t<Old>>, const New&, New&>;
};
template <typename Old, typename New>
using retain_const_t = retain_const<Old, New>::type;
// NOLINTEND(*identifier-naming)

static_assert(std::is_same_v<retain_const_t<const int, char>, const char>);
static_assert(std::is_same_v<retain_const_t<int, char>, char>);
static_assert(std::is_same_v<retain_const_t<const int, char&>, const char&>);
static_assert(std::is_same_v<retain_const_t<int, char&>, char&>);

template <std::integral IntType>
struct CommaSeperated
{
    const IntType& inner; // NOLINT
};

template <std::integral IntType>
// NOLINTNEXTLINE(*identifier-naming)
auto format_as(CommaSeperated<IntType> value) {
    std::string result;
    std::array<int, std::numeric_limits<IntType>::digits10> digits{};
    std::size_t numDigits = 0;
    for (auto val = value.inner; val > 0; val /= 10) {
        digits.at(numDigits) = static_cast<int>(val % 10);
        ++numDigits;
    }

    for (std::size_t i = 0; i < numDigits; ++i) {
        char val = static_cast<char>('0' + digits.at(i));

        if (i != 0 && i % 3 == 0) {
            result += std::string{","} + val;
        }

        result += std::string{val};
    }
    std::reverse(result.begin(), result.end());
    return result;
}

} // namespace util
