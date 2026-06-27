#pragma once

#include "company.h"

#include <algorithm>
#include <string>
#include <vector>

namespace town {

inline std::string FactoryTypeName(FactoryType type) {
    switch (type) {
        case FactoryType::Textile: return "Textile Mill";
        case FactoryType::Smelter: return "Smelter";
        case FactoryType::Food: return "Food Factory";
        case FactoryType::Brewery: return "Brewery";
        case FactoryType::Furniture: return "Furniture Works";
        case FactoryType::Jeweler: return "Jewelry Atelier";
        case FactoryType::Cartwright: return "Cartwright";
        case FactoryType::Tools: return "Toolworks";
    }
    return "Factory";
}

inline FactoryRecipe RecipeForFactory(FactoryType type, int variant = 0) {
    switch (type) {
        case FactoryType::Textile:
            return variant % 4 == 0
                ? FactoryRecipe{{{"cotton", 2.0}}, {{"cloth", 1.45}}}
                : variant % 4 == 1
                    ? FactoryRecipe{{{"wool", 1.8}}, {{"cloth", 1.25}}}
                    : variant % 4 == 2
                        ? FactoryRecipe{{{"cloth", 1.8}}, {{"clothes", 0.68}}}
                        : FactoryRecipe{{{"leather_goods", 1.1}, {"cloth", 0.7}}, {{"clothes", 0.42}}};
        case FactoryType::Smelter:
            return variant % 4 == 0
                ? ProductionOrder{{{"ore", 2.4}, {"coal", 0.35}}, {{"iron", 1.35}}}
                : variant % 4 == 1
                    ? ProductionOrder{{{"stone", 2.4}, {"charcoal", 0.25}}, {{"brick", 1.55}}}
                    : variant % 4 == 2
                        ? ProductionOrder{{{"stone", 1.8}, {"coal", 0.22}}, {{"tile", 1.20}}}
                        : ProductionOrder{{{"ore", 3.0}}, {{"silver", 0.18}, {"gold", 0.05}}};
        case FactoryType::Food:
            return variant % 3 == 0
                ? ProductionOrder{{{"grain", 1.6}}, {{"flour", 1.15}}}
                : variant % 3 == 1
                    ? ProductionOrder{{{"flour", 1.25}, {"wood", 0.25}}, {{"bread", 1.45}}}
                    : ProductionOrder{{{"grain", 1.6}, {"water", 0.45}}, {{"bread", 1.35}}};
        case FactoryType::Brewery:
            return variant % 2 == 0
                ? FactoryRecipe{{{"grain", 3.1}, {"water", 1.2}}, {{"beer", 1.45}}}
                : FactoryRecipe{{{"grain", 2.2}, {"herbs", 0.12}, {"water", 1.0}}, {{"wine", 0.72}}};
        case FactoryType::Furniture:
            return variant % 3 == 0
                ? ProductionOrder{{{"wood", 2.1}}, {{"lumber", 1.55}}}
                : variant % 3 == 1
                    ? ProductionOrder{{{"lumber", 2.2}, {"cloth", 0.22}}, {{"furniture", 0.62}}}
                    : ProductionOrder{{{"wood", 1.8}}, {{"charcoal", 1.05}}};
        case FactoryType::Jeweler:
            return variant % 4 == 0
                ? FactoryRecipe{{{"gold", 0.18}, {"silver", 0.35}}, {{"jewelry", 0.18}}}
                : variant % 4 == 1
                    ? FactoryRecipe{{{"herbs", 1.2}, {"salt", 0.35}}, {{"medicine", 0.58}}}
                    : variant % 4 == 2
                        ? FactoryRecipe{{{"herbs", 1.0}, {"spice", 0.18}}, {{"perfume", 0.42}}}
                        : FactoryRecipe{{{"silk", 0.55}, {"spice", 0.12}}, {{"clothes", 0.30}}};
        case FactoryType::Cartwright:
            return variant % 2 == 0
                ? FactoryRecipe{{{"grain", 1.4}, {"wood", 0.8}, {"leather", 0.35}}, {{"horse", 0.16}}}
                : FactoryRecipe{{{"lumber", 4.1}, {"iron", 1.4}, {"leather_goods", 0.45}}, {{"wagon", 0.14}}};
        case FactoryType::Tools:
            return variant % 5 == 0
                ? FactoryRecipe{{{"iron", 2.0}, {"wood", 1.0}}, {{"farm_tools", 0.42}}}
                : variant % 5 == 1
                    ? FactoryRecipe{{{"iron", 1.5}, {"wood", 1.0}}, {{"axe", 0.46}}}
                    : variant % 5 == 2
                        ? FactoryRecipe{{{"iron", 1.0}}, {{"water_bucket", 0.52}}}
                        : variant % 5 == 3
                            ? FactoryRecipe{{{"iron", 1.0}, {"wood", 0.45}}, {{"tools", 0.48}}}
                            : FactoryRecipe{{{"iron", 2.4}, {"wood", 0.25}}, {{"weapons", 0.18}}};
    }
    return {};
}

inline ProductionLine MakeProductionLine(const FactoryRecipe& recipe, const std::string& productId,
                                         double outputAmount, double capacityShare) {
    ProductionLine line;
    line.productId = productId;
    line.outputAmount = outputAmount;
    line.inputs = recipe.inputs;
    line.capacityShare = capacityShare;
    return line;
}

inline std::vector<ProductionLine> ProductionLinesForFactory(FactoryType type) {
    const int variantCount = type == FactoryType::Food ? 3
        : type == FactoryType::Tools ? 5
        : (type == FactoryType::Textile || type == FactoryType::Smelter || type == FactoryType::Jeweler) ? 4
        : (type == FactoryType::Furniture ? 3 : 2);
    const int desiredLines = type == FactoryType::Food || type == FactoryType::Tools ? 3 : 2;
    std::vector<ProductionLine> lines;
    lines.reserve(desiredLines);
    for (int variant = 0; variant < variantCount && static_cast<int>(lines.size()) < desiredLines; ++variant) {
        const FactoryRecipe recipe = RecipeForFactory(type, variant);
        for (const auto& [id, amount] : recipe.outputs) {
            if (amount <= 0.0) {
                continue;
            }
            const bool duplicate = std::any_of(lines.begin(), lines.end(),
                [&](const ProductionLine& line) { return line.productId == id; });
            if (!duplicate) {
                lines.push_back(MakeProductionLine(recipe, id, amount, 1.0));
                if (static_cast<int>(lines.size()) >= desiredLines) {
                    break;
                }
            }
        }
    }
    if (lines.empty()) {
        const FactoryRecipe recipe = RecipeForFactory(type, 0);
        if (!recipe.outputs.empty()) {
            lines.push_back(MakeProductionLine(recipe, recipe.outputs.front().first, recipe.outputs.front().second, 1.0));
        }
    }
    const double share = lines.empty() ? 1.0 : 1.0 / static_cast<double>(lines.size());
    for (auto& line : lines) {
        line.capacityShare = share;
    }
    return lines;
}

inline FactoryRecipe RecipeForProductionLine(const ProductionLine& line) {
    return FactoryRecipe{line.inputs, {{line.productId, line.outputAmount}}};
}

inline void RefreshFactoryMainBusinesses(Factory& factory) {
    factory.mainBusinesses.clear();
    factory.products.clear();
    for (const auto& line : factory.productionLines) {
        if (line.productId.empty()) {
            continue;
        }
        if (std::find(factory.mainBusinesses.begin(), factory.mainBusinesses.end(), line.productId) ==
            factory.mainBusinesses.end()) {
            factory.mainBusinesses.push_back(line.productId);
            factory.products.push_back(line.productId);
        }
    }
    if (factory.mainBusinesses.empty()) {
        for (const auto& [id, amount] : factory.output) {
            (void)amount;
            factory.mainBusinesses.push_back(id);
            factory.products.push_back(id);
            if (factory.mainBusinesses.size() >= 2) {
                break;
            }
        }
    }
}

inline std::vector<Factory> CreateFactories(int residentCount) {
    const FactoryType types[] = {
        FactoryType::Textile, FactoryType::Smelter, FactoryType::Food, FactoryType::Brewery,
        FactoryType::Furniture, FactoryType::Jeweler, FactoryType::Cartwright, FactoryType::Tools
    };
    std::vector<Factory> factories;
    factories.reserve(40);
    const char* names[] = {
        "Royal Weavers", "Northhill Mine", "Riverside Mill", "Saint Zeno Winery",
        "Greenwood Lumber", "Deep Sea Saltworks", "Goldmane Stables", "Old Blacksmith Guild"
    };
    for (int i = 0; i < 40; ++i) {
        Factory factory;
        factory.type = types[i % 8];
        factory.name = std::string(names[i % 8]) + " " + std::to_string(i + 1);
        factory.foundingCompany = i < 8;
        factory.majorCompany = i < 8;
        factory.owner = (i * 17) % std::max(1, residentCount);
        const int workerCount = 12 + (i * 5) % 7;
        factory.targetWorkers = static_cast<double>(workerCount);
        factory.dailyWage = 5.5;
        factory.workers.reserve(workerCount);
        for (int w = 0; w < workerCount; ++w) {
            factory.workers.push_back((i * 31 + w) % std::max(1, residentCount));
        }
        factory.efficiency = 0.88 + static_cast<double>((i * 13) % 18) / 100.0;
        factory.cash = (i < 8 ? 15000.0 : 5200.0) + static_cast<double>((i * 571) % 5001);
        factory.fixedAssets = (1600.0 + static_cast<double>(workerCount) * 95.0) * (i < 8 ? 3.0 : 1.25);
        factory.productionLines = ProductionLinesForFactory(factory.type);
        for (const auto& line : factory.productionLines) {
            const FactoryRecipe recipe = RecipeForProductionLine(line);
            const double dailyRuns = std::clamp(static_cast<double>(workerCount) / 7.0 * factory.efficiency, 0.25, 4.0);
            for (const auto& [id, amount] : recipe.inputs) {
                factory.inventory[id] = std::max(factory.inventory[id], amount * dailyRuns * line.capacityShare * 3.0);
                factory.inventoryQuality[id] = 3.0;
            }
            for (const auto& [id, amount] : recipe.outputs) {
                factory.output[id] = std::max(factory.output[id], amount * dailyRuns * line.capacityShare * 1.0);
                factory.outputQuality[id] = 3.0;
            }
        }
        RefreshFactoryMainBusinesses(factory);
        factory.shareCapital = factory.cash + factory.fixedAssets;
        factories.push_back(factory);
    }
    return factories;
}

} // namespace town

