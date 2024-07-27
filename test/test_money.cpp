#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <fmt/core.h>

#include <array>
#include <stdexcept>

#include "money_type.h"

decltype(auto) AllEq(const Money& value) {
    using Catch::Matchers::AllMatch, Catch::Matchers::Predicate;
    return AllMatch(Predicate<Money>([&](const Money& val) { return val == value; },
                                     fmt::format("All values in range should equal {}", value)));
}

TEST_CASE("Money test", "[money]") {

    SECTION("initialization equality") {
        constexpr std::array ZERO = {0_cents, 0_dollars, 0.00_dollars};
        constexpr std::array ONE_CENT = {1_cents, 0.01_dollars};
        constexpr std::array TWO_CENTS = {2_cents, 0.02_dollars};
        // TODO
        CHECK_THAT(ZERO, AllEq(Money{0, 0}));
        CHECK_THAT(ONE_CENT, AllEq(Money{0, 1}));
        CHECK_THAT(TWO_CENTS, AllEq(Money{0, 2}));

        CHECK(1.50_dollars == Money(1, 50));
        CHECK(999'999'999.99_dollars == Money(999'999'999, 99));
    }

    SECTION("exceptional cases") {
        CHECK_NOTHROW(99_cents);
        CHECK_THROWS_AS(100_cents, std::range_error);
        CHECK_THROWS_AS(101_cents, std::range_error);
        CHECK_THROWS_AS(1'000'000_cents, std::range_error);

        // More than two decimal places for ""_dollars should throw
        CHECK_NOTHROW(0._dollars);
        CHECK_NOTHROW(0.1_dollars);
        CHECK_NOTHROW(0.01_dollars);
        CHECK_THROWS_AS(0.001_dollars, std::runtime_error);
        CHECK_THROWS_AS(0.000'000'001_dollars, std::runtime_error);

        // Any dollars value out of the range of `int` should throw
        if constexpr (sizeof(int) == 4) {
            CHECK_NOTHROW(2'147'483'647.00_dollars);
            CHECK_THROWS_AS(2'147'483'648.00_dollars, std::runtime_error);
        }
        CHECK_THROWS_AS(999'999'999'999'999'999'999'999'999'999.00_dollars, std::runtime_error);
    }

    SECTION("overloaded operators") {
        // TODO
    }
}
// TODO: Constexpr tests