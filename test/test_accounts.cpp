#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <fmt/core.h>

#include "bank_account.h"
#include "cd_account.h"
#include "checking_account.h"
#include "hi_checking_account.h"
#include "hi_savings_account.h"
#include "nosc_checking_account.h"
#include "savings_account.h"
#include "sc_checking_account.h"
#include "util/date_util.h"

// To reduce syntax noise
#define repeat(times) for (int i = 0; i < times; ++i)

TEST_CASE("Simple accounts tests (no service charge and no interest)", "[account]") {
    SimTimeManager::resetDay();
    constexpr auto startingBalance = 100'000_dollars;
    const InterestHandler interestHandler(InterestType::Daily, 0.00);

    HighInterestSavingsAccount hi_savings("hi_savings", startingBalance, interestHandler, SimTimeManager{});
    NoServiceChargeCheckingAccount nosc_checking("nosc_checking", startingBalance, interestHandler, SimTimeManager{});
    HighInterestCheckingAccount hi_checking("hi_checking", startingBalance, interestHandler, SimTimeManager{});

    std::array<BankAccount, 3> all_accounts = {hi_savings, nosc_checking, hi_checking};

    auto check_balance_all = [&](const Money& expect) {
        for (const auto& account : all_accounts) {
            CAPTURE(std::string(account.getBalance()), std::string(expect));
            CHECK(account.getBalance() == expect);
        }
    };

    auto deposit_all = [&](const Money& amt) {
        for (auto& account : all_accounts) {
            account.deposit(amt);
        }
    };
    auto withdraw_all = [&](const Money& amt) {
        for (auto& account : all_accounts) {
            account.withdraw(amt);
        }
    };

    SECTION("initial balance") {
        check_balance_all(startingBalance);
    }

    SECTION("advance time with constant balance") {
        SimTimeManager::incrDay(std::chrono::days{1000});
        SimTimeManager::updateAll();
        check_balance_all(startingBalance);
    }

    SECTION("deposits and withdrawals") {
        deposit_all(500_dollars);
        check_balance_all(startingBalance + 500_dollars);
        withdraw_all(500_dollars);
        check_balance_all(startingBalance);

        repeat(1000) {
            SimTimeManager::incrDay();
            SimTimeManager::updateAll();
            withdraw_all(500_dollars);
            deposit_all(500_dollars);
        }
        check_balance_all(startingBalance);

        repeat(1000) {
            SimTimeManager::incrDay();
            SimTimeManager::updateAll();
            deposit_all(500_dollars);
        }

        check_balance_all(startingBalance + 500_dollars * 1000);
    }
}

TEST_CASE("CD account tests", "[account]") {
    SimTimeManager::resetDay();
    constexpr auto startingBalance = 100'000_dollars;
    const InterestHandler interestHandler(InterestType::Daily, 0.0);
    constexpr auto numMaturityMonths = 20;
    constexpr auto earlyWithdrawalPenalty = 0.2;

    CertificateOfDepositAccount cd_account("cd_account", startingBalance, std::chrono::months{numMaturityMonths},
                                           earlyWithdrawalPenalty, interestHandler, SimTimeManager{});

    SECTION("Inital balance and maturity") {
        CHECK(cd_account.getBalance() == startingBalance);
    }

    SECTION("Withdrawal before maturity") {
        cd_account.withdraw(50'000_dollars);
        CHECK(cd_account.getBalance() == (startingBalance - (50000_dollars * (1 + earlyWithdrawalPenalty))));
    }

    SECTION("Withdrawal after maturity") {
        SimTimeManager::incrDay(std::chrono::days{1000});
        SimTimeManager::updateAll();
        CHECK(cd_account.getBalance() == startingBalance * std::pow(1 + interestHandler.getRate(), numMaturityMonths));
    }
}

TEST_CASE("Interest tests", "[account]") {
    SimTimeManager::resetDay();
    const InterestHandler dailyInterestHandler(InterestType::Daily, 0.01);
    constexpr auto startingBalance = 100'000_dollars;

    SECTION("Daily interest accumulation") {
        SavingsAccount savings("savings", startingBalance, dailyInterestHandler, SimTimeManager{});

        SimTimeManager::incrDay(std::chrono::days{5});
        SimTimeManager::updateAll();
        auto newBalance = startingBalance;
        repeat(5) {
            newBalance *= 1 + dailyInterestHandler.getRate();
        }
        CAPTURE(std::string(savings.getBalance()), std::string(newBalance));
        CHECK(savings.getBalance() == newBalance);
    }
}

TEST_CASE("Service charge tests", "[account]") {
    SimTimeManager::resetDay();
    constexpr auto startingBalance = 100'000_dollars;
    const InterestHandler noInterestHandler(InterestType::Daily, 0.00);

    ServiceChargeCheckingAccount sc_checking("sc_checking", startingBalance, SimTimeManager{});

    SECTION("Initial balance and service charge") {
        CHECK(sc_checking.getBalance() == startingBalance);
    }

    SECTION("Service charge deduction") {
        SimTimeManager::incrDay(std::chrono::days{32});
        SimTimeManager::updateAll();
        CHECK(sc_checking.getBalance() == startingBalance - sc_checking.SERVICE_CHARGE);
        SimTimeManager::incrDay(std::chrono::days{32});
        SimTimeManager::updateAll();
        CHECK(sc_checking.getBalance() == startingBalance - sc_checking.SERVICE_CHARGE * 2);
    }
}

TEST_CASE("Monthly records tests", "[account]") {
    SimTimeManager::resetDay();
    constexpr auto startingBalance = 100'000_dollars;
    const InterestHandler noInterestHandler(InterestType::Daily, 0.00);

    auto num_records = [](const BankAccount& acc) {
        std::size_t num = 0;
        for (const auto& statement : acc.getAllMonthlyStatements()) {
            num += statement.records.size();
        }

        return num;
    };

    SECTION("count records") {
        NoServiceChargeCheckingAccount simple("simple", startingBalance, noInterestHandler, SimTimeManager{});

        // Account creation record
        CHECK(num_records(simple) == 1);

        simple.writeCheck(10_dollars);
        CHECK(num_records(simple) == 2);
        simple.deposit(10_dollars);
        CHECK(num_records(simple) == 3);
        simple.withdraw(10_dollars);
        CHECK(num_records(simple) == 4);

        // Each day should have a record to accumulate (0%) interest
        SimTimeManager::incrDay(std::chrono::days{1000});
        SimTimeManager::updateAll();
        CAPTURE(simple.getAllMonthlyStatements());

        CHECK(num_records(simple) == 1004);

        repeat(100) {
            SimTimeManager::incrDay();
            SimTimeManager::updateAll();
            simple.deposit(1_cents);
        }

        CHECK(num_records(simple) == 1204);

        // Overdraws should also create records
        repeat(10) {
            simple.withdraw(1'000'000_dollars);
            simple.writeCheck(1'000'000_dollars);
        }

        CHECK(num_records(simple) == 1224);
    }
}
