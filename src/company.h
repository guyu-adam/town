#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace town {

enum class FactoryType {
    Textile,
    Smelter,
    Food,
    Brewery,
    Furniture,
    Jeweler,
    Cartwright,
    Tools
};

struct ProductionOrder {
    std::vector<std::pair<std::string, double>> inputs;
    std::vector<std::pair<std::string, double>> outputs;
};

using FactoryRecipe = ProductionOrder;

struct ProductionLine {
    std::string productId;
    double outputAmount{0.0};
    std::vector<std::pair<std::string, double>> inputs;
    double capacityShare{1.0};
};

struct IncomeStatement {
    double salesRevenue{0.0};
    double investmentIncome{0.0};
    double rawMaterialCost{0.0};
    double wageCost{0.0};
    double depreciation{0.0};
    double interestExpense{0.0};
    double netIncome{0.0};
};

struct Company {
    FactoryType type{};
    std::string name;
    std::vector<std::string> mainBusinesses;
    std::vector<std::string> products;
    bool foundingCompany{false};
    bool majorCompany{false};
    int owner{-1};
    std::vector<int> workers;
    std::map<std::string, double> inventory;
    std::map<std::string, double> output;
    std::map<std::string, double> inventoryQuality;
    std::map<std::string, double> outputQuality;
    std::vector<ProductionLine> productionLines;
    double efficiency{1.0};

    double cash{2200.0};
    double fixedAssets{1800.0};
    double wagesPayable{0.0};
    double bankLoan{0.0};
    double shareCapital{2500.0};
    double retainedEarnings{0.0};
    double targetWorkers{8.0};
    double dailyWage{5.5};
    double targetInventoryDays{7.0};
    double overtimeLoad{0.0};
    double inventoryPressure{1.0};
    double bondsOutstanding{0.0};
    double bondCouponRate{0.065};
    int consecutiveLosses{0};
    int consecutiveProfits{0};
    int lossStreak{0};
    bool liquidating{false};
    bool bankrupt{false};
    IncomeStatement income;
    IncomeStatement quarterIncome;

    double InventoryValue(const std::map<std::string, double>& prices) const {
        double value = 0.0;
        for (const auto& [id, amount] : inventory) {
            auto found = prices.find(id);
            if (found != prices.end()) {
                value += amount * found->second;
            }
        }
        for (const auto& [id, amount] : output) {
            auto found = prices.find(id);
            if (found != prices.end()) {
                value += amount * found->second;
            }
        }
        return value;
    }

    double Assets(const std::map<std::string, double>& prices) const {
        return cash + InventoryValue(prices) + fixedAssets;
    }

    double Liabilities() const {
        return wagesPayable + bankLoan;
    }

    double Equity(const std::map<std::string, double>& prices) const {
        return Assets(prices) - Liabilities();
    }

    void BeginDay() {
        income = IncomeStatement{};
        wagesPayable = 0.0;
    }

    void CloseProfitAndLoss() {
        income.depreciation = fixedAssets * 0.00006;
        fixedAssets = std::max(0.0, fixedAssets - income.depreciation);
        income.netIncome = income.salesRevenue + income.investmentIncome -
                           income.rawMaterialCost - income.wageCost -
                           income.depreciation - income.interestExpense;
        retainedEarnings += income.netIncome;
        consecutiveLosses = income.netIncome < 0.0 ? consecutiveLosses + 1 : 0;
        consecutiveProfits = income.netIncome > 0.0 ? consecutiveProfits + 1 : 0;
        lossStreak = consecutiveLosses;
        quarterIncome.salesRevenue += income.salesRevenue;
        quarterIncome.investmentIncome += income.investmentIncome;
        quarterIncome.rawMaterialCost += income.rawMaterialCost;
        quarterIncome.wageCost += income.wageCost;
        quarterIncome.depreciation += income.depreciation;
        quarterIncome.interestExpense += income.interestExpense;
        quarterIncome.netIncome += income.netIncome;
    }

    void ResetQuarter() {
        quarterIncome = IncomeStatement{};
    }

    double DividendCapacity() const {
        if (bankrupt || income.netIncome <= 0.0 || cash < 300.0) {
            return 0.0;
        }
        return std::min(cash - 300.0, income.netIncome * 0.25);
    }
};

using Factory = Company;

} // namespace town
