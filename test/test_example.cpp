#include <gtest/gtest.h>

#include <array>
#include <cmath>

constexpr std::array primes = {2, 3, 5, 7, 11, 13};

TEST(Primes, VerifyPrimes) {
    for (auto maybe_prime : primes) {
        int num_factors = 0;
        const int sqrt_prime = static_cast<int>(std::sqrt(maybe_prime));

        for (int i = 2; i <= sqrt_prime; i++) {
            if (maybe_prime % i == 0) {
                ++num_factors;
            }
        }

        // There should be no factors in range [2, sqrt(num)]
        EXPECT_EQ(num_factors, 0) << maybe_prime << " has multiple factors!";
    }
}