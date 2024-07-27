#include "console_interface.h"
#include "bank_account.h"
#include "cd_account.h"
#include "hi_checking_account.h"
#include "hi_savings_account.h"
#include "interest_handler.h"
#include "nosc_checking_account.h"
#include "savings_account.h"
#include "sc_checking_account.h"
#include "util/date_util.h"

#include <fmt/color.h>
#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

template <typename... Args>
void errorMsg(std::string_view msg, Args&&... args) {
    fmt::print(fmt::fg(fmt::color::red), msg, std::forward<Args>(args)...);
    fmt::println("");
}
template <typename... Args>
void successMsg(std::string_view msg, Args&&... args) {
    fmt::print(fmt::fg(fmt::color::green), msg, std::forward<Args>(args)...);
    fmt::println("");
}

std::optional<int> getIntInput() {
    std::string rawInput;
    std::getline(std::cin, rawInput);
    int result{};

    try {
        result = std::stoi(rawInput);
    } catch (std::out_of_range&) {
        errorMsg("Integer value is out of range!");
        return std::nullopt;
    } catch (std::invalid_argument&) {
        errorMsg("Not a valid integer!");
        return std::nullopt;
    }

    return std::optional{result};
}

} // namespace

void ConsoleInterface::run() {
    using enum MainMenuOption;
    using enum AccountMenuOption;

    for (;;) {
        BankAccount* selected = nullptr;
        std::optional<int> numSelection;
        AccountMenuOption accountAction{};

        switch (mainMenu()) {
        case StepDays:
            std::cout << "Enter how many days would you like to step by: ";
            numSelection = getIntInput();
            if (!numSelection.has_value()) {
                errorMsg("Invalid input!");
                break;
            }
            if (numSelection.value() <= 0) {
                errorMsg("Number of days must be positive!");
                break;
            }

            SimTimeManager::incrDay(std::chrono::days{numSelection.value()});
            SimTimeManager::updateAll();

            successMsg("Stepped days forward by {}. The date is now {:%D}", numSelection.value(),
                       SimTimeManager::getDate());
            break;

        case NewAccount:
            newAccount();
            break;

        case SelectAccount:
            selected = selectAccount();
            if (selected == nullptr) {
                break;
            }
            accountAction = accountMenu(*selected);
            handleAccountAction(accountAction, *selected);
            break;
        case Quit:
            return;
        }
    }
}

// enum class MainMenuOption { NewAccount, StepDays, SelectAccount, Quit };

ConsoleInterface::MainMenuOption ConsoleInterface::mainMenu() const {
    bool selectionValid = false;
    std::optional<int> selection;

    while (!selectionValid) {
        std::cout << "Please select an option:"
                  << "\n\t1) New Account"
                  << "\n\t2) Simulate Time"
                  << "\n\t3) Account Actions"
                  << "\n\t4) Quit"
                  << "\n[1-4]: ";

        selection = getIntInput();
        auto inRangeFn = [](int val) { return val > 0 && val < 5; };

        if (!selection.has_value() || !inRangeFn(selection.value())) {
            errorMsg("Invalid option!");
            continue;
        }

        selectionValid = true;
    }

    return static_cast<MainMenuOption>(selection.value() - 1); // NOLINT
}

// enum class AccountMenuOption { Close, DisplayStatement, Info, Deposit, Withdraw, Back };

ConsoleInterface::AccountMenuOption ConsoleInterface::accountMenu(const BankAccount& account) const {
    // TODO: Refactor duplicate code in other menu function
    std::optional<int> selection;

    std::cout << fmt::format("Please select an option for account {}:", account.getAccountNumber())
              << "\n\t1) Close Account"
              << "\n\t2) Display Statements"
              << "\n\t3) Account Info"
              << "\n\t4) Deposit Funds"
              << "\n\t5) Withdraw Funds"
              << "\n[1-5, or any other value to go back]: ";

    selection = getIntInput();
    auto inRangeFn = [](int val) { return val > 0 && val < 6; };

    if (!selection.has_value() || !inRangeFn(selection.value())) {
        successMsg("Returning to main menu.");
        return AccountMenuOption::Back;
    }

    return static_cast<AccountMenuOption>(selection.value() - 1);
}

void ConsoleInterface::handleAccountAction(AccountMenuOption opt, BankAccount& account) {
    using enum AccountMenuOption;

    // Not using switch to reduce nesting and help readability

    if (opt == Back) {
        return;
    }

    if (opt == Close) {
        successMsg("Successfully closed account.");
        std::erase_if(openAccounts_, [&](auto& acc) { return account.getAccountNumber() == acc.getAccountNumber(); });
        return;
    }

    if (opt == DisplayStatement) {
        successMsg("Account statements below:\n");
        fmt::println("{}", fmt::join(account.getAllMonthlyStatements(), "\n\n"));
        return;
    }

    if (opt == Info) {
        successMsg("Current account info below:");
        fmt::println("Bank account number {1}:"
                     "\n\tOwned by {0:?}"
                     "\n\tOpened on {2:%D}"
                     "\n\tCurrent balance is {3}"
                     "\n\n",
                     account.getAccountName(), account.getAccountNumber(), account.getAccountOpeningDate(),
                     account.getBalance());
        return;
    }

    std::optional<int> selectedAmount;
    if (opt == Deposit || opt == Withdraw) {
        std::cout << "Please specify an amount (USD): ";
        selectedAmount = getIntInput();
    } else {
        errorMsg("Internal error; invalid option.");
        return;
    }

    if (!selectedAmount.has_value()) {
        errorMsg("Invalid input! Must be integral.");
        return;
    }

    if (opt == Deposit) {
        successMsg("Deposited ${}.", selectedAmount.value());
        account.deposit(Money{selectedAmount.value(), 0});
    } else {
        successMsg("Withdrew ${}.", selectedAmount.value());
        account.withdraw(Money{selectedAmount.value(), 0});
    }
}

BankAccount* ConsoleInterface::selectAccount() {
    std::vector<int> accountIds;
    std::transform(openAccounts_.begin(), openAccounts_.end(), std::back_inserter(accountIds),
                   [](const auto& acc) { return acc.getAccountNumber(); });

    fmt::print("Please choose an account number from the following: [{}]\nid: ", fmt::join(accountIds, ", "));

    auto choice = getIntInput();

    if (!choice.has_value()) {
        errorMsg("Invalid input!");
        return nullptr;
    }

    if (auto iter = std::find(accountIds.begin(), accountIds.end(), choice); iter != accountIds.end()) {
        // The index of each corresponding id will remain the same
        auto idx = static_cast<std::size_t>(std::distance(accountIds.begin(), iter));
        return &openAccounts_[idx];
    }

    errorMsg("Account id does not exist!");
    return nullptr;
}

void ConsoleInterface::newAccount() {
    std::cout << "Please select account type:"
              << "\n\t1) Certificate of Deposit"
              << "\n\t2) No Service Charge Checking"
              << "\n\t3) Service Charge Checking"
              << "\n\t4) High Interest Checking"
              << "\n\t5) Savings"
              << "\n\t6) High Interest Savings"
              << "\n[1-5, or any other number to go back]: ";

    auto selection = getIntInput();
    auto inRangeFn = [](int val) { return val > 0 && val < 7; };

    if (!selection.has_value() || !inRangeFn(selection.value())) {
        successMsg("Returning to main menu.");
        return;
    }

    constexpr auto startingBalance = 15'000.00_dollars;
    constexpr auto maturityMonths = std::chrono::months{6};
    constexpr auto withdrawalPenalty = 0.2;
    InterestHandler defaultInterest(InterestType::Monthly, 0.05);
    constexpr std::array<std::string_view, 5> holderNames = {"Bob Joe II", "William Bill", "King Edward", "Darth Vader",
                                                             "Aragorn"};
    std::string name{[&] {
        std::uniform_int_distribution<> distr(0, holderNames.size() - 1);
        std::mt19937 gen{std::random_device{}()};
        return holderNames.at(static_cast<std::size_t>(distr(gen)));
    }()};

    switch (selection.value()) {
    case 1:
        openAccounts_.emplace_back(CertificateOfDepositAccount(name, startingBalance, maturityMonths, withdrawalPenalty,
                                                               defaultInterest, SimTimeManager{}));
        break;
    case 2:
        openAccounts_.emplace_back(
            NoServiceChargeCheckingAccount(name, startingBalance, defaultInterest, SimTimeManager{}));
        break;
    case 3:
        openAccounts_.emplace_back(ServiceChargeCheckingAccount(name, startingBalance, SimTimeManager{}));
        break;
    case 4:
        openAccounts_.emplace_back(
            HighInterestCheckingAccount(name, startingBalance, defaultInterest, SimTimeManager{}));
        break;
    case 5:
        openAccounts_.emplace_back(SavingsAccount(name, startingBalance, defaultInterest, SimTimeManager{}));
        break;
    case 6:
        openAccounts_.emplace_back(
            HighInterestSavingsAccount(name, startingBalance, defaultInterest, SimTimeManager{}));
        break;

    default:
        // Should be unreachable
        assert(false);
        break;
    }
}