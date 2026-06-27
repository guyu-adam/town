#pragma once

#include <algorithm>

namespace town {

struct WaterSystem {
    double wellStock{620.0};
    double riverFlow{145.0};
    double dailyNeedPerResident{1.0};
    double shortage{0.0};
    int consecutiveShortageDays{0};
};

inline double RefillWater(WaterSystem& water, int seasonIndex) {
    const double seasonFlow = seasonIndex == 3 ? 0.75 : (seasonIndex == 1 ? 1.20 : 1.0);
    const double refill = 360.0 * seasonFlow + water.riverFlow * 0.35;
    water.wellStock = std::min(3600.0, water.wellStock + refill);
    return refill;
}

inline double DrawWater(WaterSystem& water, double requested) {
    const double taken = std::min(water.wellStock, std::max(0.0, requested));
    water.wellStock -= taken;
    water.shortage = std::max(0.0, requested - taken);
    if (water.shortage > 0.01) {
        ++water.consecutiveShortageDays;
    } else {
        water.consecutiveShortageDays = 0;
    }
    return taken;
}

} // namespace town

