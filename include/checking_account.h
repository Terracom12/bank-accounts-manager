/*! \file checking_account.h
    \brief File containing the CheckingAccount class and concept

    The CheckingAccount class and its nested classes, Concept and Model, as well as CheckingAccountConcept
    are declared within this header file
*/
#pragma once

#include "bank_account.h"
#include "money_type.h"
#include "monthly_statement.h"
#include "util/date_util.h"

#include <memory>
#include <string_view>
#include <vector>

//! Concept for CheckingAccount to be used with the Model class template
/*!
    This concept constrains the writeCheck member function
*/
template <typename T>
concept CheckingAccountConcept = BankAccountConcept<T> && requires(T acc) {
    { acc.writeCheck(Money{}) } -> std::same_as<void>;
};

//! Manages bank account actions implemented within the different checking accounts
/*!
    Functions similarily to BankAccount but implements the writeCheck member function
*/
class CheckingAccount
{
public:
    template <typename AccountType>
        requires(!std::same_as<AccountType, CheckingAccount>) && CheckingAccountConcept<AccountType>
    /*implicit*/ CheckingAccount(const AccountType& account) // NOLINT
        : pimpl_(std::make_unique<Model<AccountType>>(account)) {}

    CheckingAccount(const CheckingAccount& other)
        : pimpl_(dynamic_cast<Concept*>(other.pimpl_->clone().release())) {}

    CheckingAccount& operator=(const CheckingAccount& rhs) {
        if (&rhs == this) {
            return *this;
        }
        pimpl_.reset(rhs.pimpl_.get());
        return *this;
    }
    CheckingAccount(CheckingAccount&&) = default;
    CheckingAccount& operator=(CheckingAccount&&) = default;
    ~CheckingAccount() = default;

    std::string_view getAccountName() const { return pimpl_->getAccountName(); };
    int getAccountNumber() const { return pimpl_->getAccountNumber(); };
    Date getAccountOpeningDate() const { return pimpl_->getAccountOpeningDate(); };
    Money getBalance() const { return pimpl_->getBalance(); };
    const MonthlyStatement& getMonthlyStatement(const Date& when) const { return pimpl_->getMonthlyStatement(when); };
    const std::vector<MonthlyStatement>& getAllMonthlyStatements() const { return pimpl_->getAllMonthlyStatements(); };
    void deposit(const Money& amount) { pimpl_->deposit(amount); };
    void withdraw(const Money& amount) { pimpl_->withdraw(amount); };
    void writeCheck(const Money& amount) { pimpl_->writeCheck(amount); };

private:
    //! Abstract class inheriting from BankAccount
    /*!
      The abstract class Concept inherits member functions from BankAccount and adds a new member function called
      writeCheck
    */
    class Concept : public virtual BankAccount::Concept
    {
    public:
        virtual void writeCheck(const Money& amount) = 0;
    };

    //! Template argument for CheckingAccountConcept inheriting from Concept and BankAccount
    /*!
      This class is nested within CheckingAccount and overrides the Concept class as well while inheriting
      from BankAccount the same as COncept does
    */
    template <CheckingAccountConcept AccountType>
    class Model : public Concept, public BankAccount::Model<AccountType>
    {
    public:
        explicit Model(const AccountType& account)
            : BankAccount::Model<AccountType>(account) {}

        std::unique_ptr<BankAccount::Concept> clone() override { return std::make_unique<Model>(*this); };

        void writeCheck(const Money& amount) override { BankAccount::Model<AccountType>::impl_.writeCheck(amount); };
    };

    std::unique_ptr<Concept> pimpl_;
};
