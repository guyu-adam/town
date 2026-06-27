#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace town {

enum class BorrowerKind {
    Resident,
    Company,
    Interbank
};

struct Loan {
    BorrowerKind kind{BorrowerKind::Resident};
    int borrowerId{-1};
    double principal{0.0};
    double annualRate{0.08};
    double collateral{0.0};
    int daysPastDue{0};
    bool nonPerforming{false};
};

class CommercialBank {
public:
    std::string name{"Town Commercial Bank"};
    double deposits{0.0};
    double loans{0.0};
    double reserves{0.0};
    double equity{28000.0};
    double loanLossAllowance{0.0};
    double interbankBorrowings{0.0};
    double interbankAssets{0.0};
    double reserveRequirement{0.10};
    double depositRate{0.018};
    double primeLoanRate{0.082};
    bool failed{false};
    std::map<int, double> residentDeposits;
    std::map<int, double> companyDeposits;
    std::vector<Loan> loanBook;

    void OpenResidentAccount(int id, double initialDeposit) {
        if (residentDeposits.count(id) == 0) {
            const double amount = std::max(0.0, initialDeposit);
            residentDeposits[id] = amount;
            deposits += amount;
            reserves += amount;
        }
    }

    void OpenCompanyAccount(int id, double initialDeposit) {
        if (companyDeposits.count(id) == 0) {
            const double amount = std::max(0.0, initialDeposit);
            companyDeposits[id] = amount;
            deposits += amount;
            reserves += amount;
        }
    }

    double ExcessReserves() const {
        return reserves - deposits * reserveRequirement;
    }

    double CapitalAdequacyRatio() const {
        return loans > 0.01 ? std::max(0.0, equity - loanLossAllowance) / loans : 1.0;
    }

    double NonPerformingLoanRate() const {
        if (loanBook.empty()) {
            return 0.0;
        }
        double bad = 0.0;
        double outstanding = 0.0;
        for (const auto& loan : loanBook) {
            outstanding += loan.principal;
            if (loan.nonPerforming) {
                bad += loan.principal;
            }
        }
        return outstanding > 0.01 ? bad / outstanding : 0.0;
    }

    bool ApproveLoan(BorrowerKind kind, int borrowerId, double amount, double annualIncome,
                     double assets, double collateral) const {
        (void)kind;
        (void)borrowerId;
        const double lendableCapacity = std::max(0.0, ExcessReserves()) / std::max(0.04, reserveRequirement) * 0.72;
        if (failed || amount <= 0.0 || lendableCapacity < amount || CapitalAdequacyRatio() < 0.065) {
            return false;
        }
        const double rateTightness = 1.0 + std::max(0.0, primeLoanRate - 0.055) * 5.0;
        const double creditTightness = (1.0 + NonPerformingLoanRate() * 4.0 +
                                        std::max(0.0, 0.10 - CapitalAdequacyRatio()) * 6.0) * rateTightness;
        const double coverage = annualIncome * 1.8 + assets * 0.26 + collateral * 0.62;
        const double annualDebtService = amount * std::max(0.001, primeLoanRate);
        return coverage >= (amount * 0.36 + annualDebtService * 1.7) * creditTightness;
    }

    bool IssueLoan(BorrowerKind kind, int borrowerId, double amount, double annualIncome,
                   double assets, double collateral) {
        if (!ApproveLoan(kind, borrowerId, amount, annualIncome, assets, collateral)) {
            return false;
        }
        loanBook.push_back(Loan{kind, borrowerId, amount, primeLoanRate, collateral, 0, false});
        loans += amount;
        deposits += amount;
        if (kind == BorrowerKind::Resident) {
            residentDeposits[borrowerId] += amount;
        } else {
            companyDeposits[borrowerId] += amount;
        }
        return true;
    }

    void DepositResident(int id, double amount) {
        if (failed) return;
        if (amount <= 0.0) return;
        residentDeposits[id] += amount;
        deposits += amount;
        reserves += amount;
    }

    bool WithdrawResident(int id, double amount) {
        if (failed) return false;
        if (amount <= 0.0) return true;
        double& account = residentDeposits[id];
        const double paid = std::min(account, amount);
        account -= paid;
        deposits -= paid;
        reserves -= paid;
        return paid >= amount - 0.001;
    }

    void DepositCompany(int id, double amount) {
        if (failed) return;
        if (amount <= 0.0) return;
        companyDeposits[id] += amount;
        deposits += amount;
        reserves += amount;
    }

    bool WithdrawCompany(int id, double amount) {
        if (failed) return false;
        if (amount <= 0.0) return true;
        double& account = companyDeposits[id];
        const double paid = std::min(account, amount);
        account -= paid;
        deposits -= paid;
        reserves -= paid;
        return paid >= amount - 0.001;
    }

    template <typename ResidentCollection>
    void PayDepositInterest(ResidentCollection& residents, double policyRate, int daysPerYear) {
        if (failed) {
            for (auto& resident : residents) {
                resident.savings = 0.0f;
            }
            return;
        }
        depositRate = std::max(0.001, policyRate * 0.52);
        const double dailyRate = depositRate / static_cast<double>(daysPerYear);
        for (auto& resident : residents) {
            const double balance = residentDeposits[resident.id];
            const double interest = balance * dailyRate;
            residentDeposits[resident.id] += interest;
            resident.savings = residentDeposits[resident.id];
            resident.money += interest;
            resident.dailyDepositInterest = interest;
            deposits += interest;
            reserves -= interest;
        }
        equity -= std::max(0.0, deposits * dailyRate * 0.03);
    }

    template <typename CompanyCollection>
    void AccrueLoanInterest(CompanyCollection& companies, int daysPerYear) {
        if (failed) return;
        for (auto& loan : loanBook) {
            const double interest = loan.principal * loan.annualRate / static_cast<double>(daysPerYear);
            if (loan.kind == BorrowerKind::Company &&
                loan.borrowerId >= 0 && loan.borrowerId < static_cast<int>(companies.size())) {
                auto& company = companies[loan.borrowerId];
                if (company.bankrupt) {
                    loan.nonPerforming = true;
                    ++loan.daysPastDue;
                    continue;
                }
                const double paid = std::min(company.cash, interest);
                company.cash -= paid;
                company.income.interestExpense += paid;
                reserves += paid;
                equity += paid;
                if (paid + 0.001 < interest) {
                    ++loan.daysPastDue;
                    loan.nonPerforming = loan.daysPastDue > 30;
                } else {
                    loan.daysPastDue = 0;
                    loan.nonPerforming = false;
                }
            }
        }
        loanLossAllowance = loans * NonPerformingLoanRate() * 0.55;
        equity -= loanLossAllowance * 0.01;
    }

    void SettleInterbank(std::vector<CommercialBank*>& banks) {
        if (failed) return;
        for (auto* borrower : banks) {
            if (!borrower || borrower->ExcessReserves() >= 0.0) continue;
            const double needed = -borrower->ExcessReserves();
            for (auto* lender : banks) {
                if (!lender || lender == borrower || lender->ExcessReserves() <= 0.0) continue;
                const double amount = std::min(needed, lender->ExcessReserves() * 0.5);
                if (amount <= 0.0) continue;
                lender->reserves -= amount;
                lender->interbankAssets += amount;
                borrower->reserves += amount;
                borrower->interbankBorrowings += amount;
                break;
            }
        }
    }
};

} // namespace town
