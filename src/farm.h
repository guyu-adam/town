#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace town {

enum class CropType {
    Grain,
    Cotton
};

struct FieldPlot {
    CropType crop{CropType::Grain};
    double growth{0.0};
    double fertility{1.0};
};

struct HouseholdFarm {
    std::vector<FieldPlot> plots;
    double labor{1.0};
};

inline std::string CropGoodId(CropType crop) {
    return crop == CropType::Grain ? "grain" : "cotton";
}

inline std::string SeedGoodId(CropType crop) {
    return crop == CropType::Grain ? "seed_grain" : "seed_cotton";
}

inline double FarmSeasonMultiplier(int seasonIndex) {
    switch (seasonIndex) {
        case 0: return 1.20;
        case 1: return 1.35;
        case 2: return 1.70;
        case 3: return 0.42;
    }
    return 1.0;
}

inline HouseholdFarm CreateHouseholdFarm(int householdId) {
    HouseholdFarm farm;
    const int count = 1 + householdId % 3;
    farm.plots.reserve(count);
    for (int i = 0; i < count; ++i) {
        FieldPlot plot;
        plot.crop = ((householdId + i) % 5 == 0) ? CropType::Cotton : CropType::Grain;
        plot.growth = 0.15 + 0.18 * static_cast<double>((householdId + i) % 4);
        plot.fertility = 0.86 + 0.07 * static_cast<double>((householdId + i) % 5);
        farm.plots.push_back(plot);
    }
    farm.labor = 0.85 + 0.05 * static_cast<double>(householdId % 7);
    return farm;
}

inline std::map<std::string, double> AdvanceHouseholdFarm(HouseholdFarm& farm, int seasonIndex,
                                                          double toolBonus, double laborMultiplier) {
    std::map<std::string, double> output;
    const double season = FarmSeasonMultiplier(seasonIndex);
    for (auto& plot : farm.plots) {
        plot.growth = std::min(1.0, plot.growth + 0.18 * season * toolBonus * laborMultiplier * farm.labor);
        if (plot.growth >= 1.0) {
            const double baseYield = plot.crop == CropType::Grain ? 3.4 : 2.1;
            output[CropGoodId(plot.crop)] += baseYield * plot.fertility * season * toolBonus;
            plot.growth = 0.08;
        }
    }
    return output;
}

} // namespace town

