#pragma once

#include "raylib.h"
#include "vendor/FastNoiseLite.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace town {

enum class TerrainBiome : unsigned char {
    DeepWater,
    ShallowWater,
    Sand,
    Grass,
    Forest,
    Stone,
    Snow,
    Ocean = DeepWater,
    Beach = Sand,
    Plains = Grass,
    Hills = 7,
    Mountains = 8,
    Farmland = 9
};

enum class TerrainResource : unsigned char {
    None,
    Iron,
    Gold,
    Silver,
    Platinum,
    Coal,
    Salt,
    Wood,
    FertileSoil,
    Water
};

struct TerrainTile {
    float height{};
    unsigned char altitude{};
    float moisture{};
    float fertility{};
    TerrainBiome biome{TerrainBiome::Grass};
    TerrainResource resource{TerrainResource::None};
    float reserve{};
    float maxReserve{};
    bool river{};
};

struct TerrainMap {
    static constexpr int TileSize = 32;
    static constexpr int Columns = 400;
    static constexpr int Rows = 300;
    static constexpr int PixelWidth = Columns * TileSize;
    static constexpr int PixelHeight = Rows * TileSize;

    std::vector<TerrainTile> tiles;
};

inline int TerrainIndex(int col, int row) {
    return row * TerrainMap::Columns + col;
}

inline bool InTerrainBounds(int col, int row) {
    return col >= 0 && col < TerrainMap::Columns && row >= 0 && row < TerrainMap::Rows;
}

inline TerrainTile& TerrainAt(TerrainMap& map, int col, int row) {
    return map.tiles[TerrainIndex(col, row)];
}

inline const TerrainTile& TerrainAt(const TerrainMap& map, int col, int row) {
    return map.tiles[TerrainIndex(col, row)];
}

inline float TerrainClamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

inline float TerrainNoise01(const FastNoiseLite& noise, float x, float y) {
    return TerrainClamp01(noise.GetNoise(x, y) * 0.5f + 0.5f);
}

inline TerrainBiome PickBiome(float height, float moisture) {
    const int altitude = static_cast<int>(std::clamp(height, 0.0f, 1.0f) * 255.0f);
    if (altitude < 70) return TerrainBiome::DeepWater;
    if (altitude < 90) return TerrainBiome::ShallowWater;
    if (altitude < 110) return TerrainBiome::Sand;
    if (altitude < 150) return TerrainBiome::Grass;
    if (altitude < 200) return TerrainBiome::Forest;
    if (altitude < 220) return TerrainBiome::Stone;
    if (altitude > 220) return TerrainBiome::Snow;
    if (moisture > 0.66f) return TerrainBiome::Forest;
    return TerrainBiome::Grass;
}

inline void StampRiver(TerrainMap& map, int col, int row) {
    for (int yy = -1; yy <= 1; ++yy) {
        for (int xx = -1; xx <= 1; ++xx) {
            const int x = col + xx;
            const int y = row + yy;
            if (!InTerrainBounds(x, y)) continue;
            TerrainTile& tile = TerrainAt(map, x, y);
            tile.river = true;
            tile.resource = TerrainResource::Water;
            tile.reserve = tile.maxReserve = 1000000.0f;
            if (tile.biome != TerrainBiome::DeepWater && tile.biome != TerrainBiome::ShallowWater && tile.biome != TerrainBiome::Sand) {
                tile.moisture = std::max(tile.moisture, 0.82f);
                tile.fertility = std::max(tile.fertility, 0.78f);
            }
        }
    }
}

inline void GenerateRivers(TerrainMap& map) {
    const int starts[][2] = {{92, 28}, {238, 38}, {330, 72}, {172, 92}, {286, 116}};
    for (const auto& start : starts) {
        int col = start[0];
        int row = start[1];
        for (int step = 0; step < 720 && InTerrainBounds(col, row); ++step) {
            StampRiver(map, col, row);
            if ((TerrainAt(map, col, row).biome == TerrainBiome::DeepWater ||
                 TerrainAt(map, col, row).biome == TerrainBiome::ShallowWater) && step > 16) break;

            int bestCol = col;
            int bestRow = row;
            float bestScore = TerrainAt(map, col, row).height + static_cast<float>(row) * 0.0028f;
            for (int yy = -1; yy <= 1; ++yy) {
                for (int xx = -1; xx <= 1; ++xx) {
                    if (xx == 0 && yy == 0) continue;
                    const int nx = col + xx;
                    const int ny = row + yy;
                    if (!InTerrainBounds(nx, ny)) continue;
                    const float southBias = static_cast<float>(ny) * 0.0028f;
                    const float meander = std::sin(static_cast<float>(nx) * 0.11f + static_cast<float>(step) * 0.19f) * 0.012f;
                    const float score = TerrainAt(map, nx, ny).height - southBias + meander;
                    if (score < bestScore || (bestCol == col && bestRow == row && ny > row)) {
                        bestScore = score;
                        bestCol = nx;
                        bestRow = ny;
                    }
                }
            }
            if (bestCol == col && bestRow == row) {
                bestRow = std::min(TerrainMap::Rows - 1, row + 1);
            }
            col = bestCol;
            row = bestRow;
        }
    }
}

inline TerrainResource ClusterOreType(int seed) {
    switch (std::abs(seed) % 5) {
        case 0: return TerrainResource::Iron;
        case 1: return TerrainResource::Gold;
        case 2: return TerrainResource::Silver;
        case 3: return TerrainResource::Platinum;
        default: return TerrainResource::Coal;
    }
}

inline bool IsStoneLayer(TerrainBiome biome) {
    return biome == TerrainBiome::Stone || biome == TerrainBiome::Snow || biome == TerrainBiome::Mountains;
}

inline void StampOreCluster(TerrainMap& map, int col, int row, TerrainResource ore, int radius, float richness) {
    for (int yy = -radius; yy <= radius; ++yy) {
        for (int xx = -radius; xx <= radius; ++xx) {
            const int x = col + xx;
            const int y = row + yy;
            if (!InTerrainBounds(x, y)) continue;
            if (std::abs(xx) + std::abs(yy) > radius + 1) continue;
            TerrainTile& tile = TerrainAt(map, x, y);
            if (!IsStoneLayer(tile.biome)) continue;
            tile.resource = ore;
            tile.reserve = tile.maxReserve = 2400.0f + richness * 8200.0f;
        }
    }
}

inline void PlaceTerrainResources(TerrainMap& map, const FastNoiseLite& oreNoise) {
    for (int row = 0; row < TerrainMap::Rows; ++row) {
        for (int col = 0; col < TerrainMap::Columns; ++col) {
            TerrainTile& tile = TerrainAt(map, col, row);
            if (tile.biome == TerrainBiome::Forest) {
                tile.resource = TerrainResource::Wood;
                tile.reserve = tile.maxReserve = 360.0f + tile.moisture * 520.0f;
            } else if ((tile.biome == TerrainBiome::Grass || tile.biome == TerrainBiome::Sand) && tile.fertility > 0.67f) {
                tile.biome = TerrainBiome::Farmland;
                tile.resource = TerrainResource::FertileSoil;
                tile.reserve = tile.maxReserve = 220.0f + tile.fertility * 440.0f;
            }

            if (IsStoneLayer(tile.biome)) {
                const float ore = TerrainNoise01(oreNoise, static_cast<float>(col) * 1.7f, static_cast<float>(row) * 1.7f);
                if (ore > 0.86f && ((col * 31 + row * 17) % 11 == 0)) {
                    const int radius = 1 + ((col * 13 + row * 7) % 3);
                    StampOreCluster(map, col, row, ClusterOreType(col * 92821 + row * 68917), radius, ore);
                }
            }

            if ((tile.biome == TerrainBiome::Sand || tile.biome == TerrainBiome::ShallowWater) && tile.height > 0.27f &&
                tile.height < 0.39f && TerrainNoise01(oreNoise, col * 4.1f + 91.0f, row * 4.1f) > 0.76f) {
                tile.resource = TerrainResource::Salt;
                tile.reserve = tile.maxReserve = 900.0f + tile.moisture * 1900.0f;
            }
        }
    }
}

inline TerrainMap CreateTerrainMap(int seed = 20260624) {
    TerrainMap map;
    map.tiles.resize(TerrainMap::Columns * TerrainMap::Rows);

    FastNoiseLite heightNoise(seed);
    heightNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    heightNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    heightNoise.SetFrequency(0.0064f);
    heightNoise.SetFractalOctaves(5);
    heightNoise.SetFractalLacunarity(2.08f);
    heightNoise.SetFractalGain(0.48f);

    FastNoiseLite moistureNoise(seed + 71);
    moistureNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    moistureNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    moistureNoise.SetFrequency(0.0115f);
    moistureNoise.SetFractalOctaves(4);
    moistureNoise.SetFractalGain(0.54f);

    FastNoiseLite oreNoise(seed + 173);
    oreNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    oreNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    oreNoise.SetFrequency(0.024f);
    oreNoise.SetFractalOctaves(3);

    const float cx = static_cast<float>(TerrainMap::Columns) * 0.54f;
    const float cy = static_cast<float>(TerrainMap::Rows) * 0.44f;
    const float maxDist = std::sqrt(cx * cx + cy * cy);

    for (int row = 0; row < TerrainMap::Rows; ++row) {
        for (int col = 0; col < TerrainMap::Columns; ++col) {
            const float x = static_cast<float>(col);
            const float y = static_cast<float>(row);
            const float continent = TerrainNoise01(heightNoise, x, y);
            const float dx = (x - cx) / maxDist;
            const float dy = (y - cy) / maxDist;
            const float islandFalloff = TerrainClamp01(1.0f - std::sqrt(dx * dx + dy * dy) * 1.52f);
            const float coastShelf = TerrainNoise01(heightNoise, x * 0.29f + 800.0f, y * 0.29f - 220.0f) * 0.12f;
            const float height = TerrainClamp01(continent * 0.68f + islandFalloff * 0.48f + coastShelf - 0.10f);
            const float moisture = TerrainClamp01(TerrainNoise01(moistureNoise, x, y) * 0.76f + (1.0f - height) * 0.22f);

            TerrainTile& tile = TerrainAt(map, col, row);
            tile.height = height;
            tile.altitude = static_cast<unsigned char>(std::clamp(height, 0.0f, 1.0f) * 255.0f);
            tile.moisture = moisture;
            tile.fertility = TerrainClamp01((1.0f - std::abs(height - 0.50f) * 2.0f) * 0.62f + moisture * 0.44f);
            tile.biome = PickBiome(height, moisture);
        }
    }

    GenerateRivers(map);
    PlaceTerrainResources(map, oreNoise);
    return map;
}

inline TerrainMap& GetMutableTerrainMap() {
    static TerrainMap map = CreateTerrainMap();
    return map;
}

inline const TerrainMap& GetTerrainMap() {
    return GetMutableTerrainMap();
}

inline Color TerrainBiomeColor(TerrainBiome biome) {
    switch (biome) {
        case TerrainBiome::Ocean: return Color{54, 125, 196, 255};
        case TerrainBiome::ShallowWater: return Color{74, 154, 204, 255};
        case TerrainBiome::Beach: return Color{221, 204, 124, 255};
        case TerrainBiome::Plains: return Color{116, 187, 82, 255};
        case TerrainBiome::Hills: return Color{100, 160, 78, 255};
        case TerrainBiome::Mountains: return Color{130, 132, 126, 255};
        case TerrainBiome::Stone: return Color{119, 122, 119, 255};
        case TerrainBiome::Snow: return Color{222, 229, 226, 255};
        case TerrainBiome::Forest: return Color{47, 139, 60, 255};
        case TerrainBiome::Farmland: return Color{184, 190, 77, 255};
    }
    return MAGENTA;
}

inline Color TerrainResourceColor(TerrainResource resource) {
    switch (resource) {
        case TerrainResource::Iron: return Color{70, 72, 75, 255};
        case TerrainResource::Gold: return Color{218, 171, 48, 255};
        case TerrainResource::Silver: return Color{226, 230, 224, 255};
        case TerrainResource::Platinum: return Color{143, 163, 174, 255};
        case TerrainResource::Salt: return Color{232, 225, 196, 255};
        case TerrainResource::Coal: return Color{38, 36, 34, 255};
        case TerrainResource::Wood: return Color{30, 83, 44, 255};
        case TerrainResource::FertileSoil: return Color{207, 178, 72, 255};
        case TerrainResource::Water: return Color{72, 155, 205, 255};
        case TerrainResource::None: break;
    }
    return BLANK;
}

inline Color ApplyTerrainShade(Color base, const TerrainTile& tile) {
    const float shade = (tile.height - 0.5f) * 18.0f + (tile.moisture - 0.5f) * 8.0f;
    auto channel = [shade](unsigned char c) {
        return static_cast<unsigned char>(std::clamp(static_cast<int>(c) + static_cast<int>(shade), 0, 255));
    };
    return Color{channel(base.r), channel(base.g), channel(base.b), 255};
}

inline void DrawTerrainMap(const TerrainMap& map, Rectangle visibleWorld) {
    const int startCol = std::max(0, static_cast<int>(std::floor(visibleWorld.x / TerrainMap::TileSize)) - 1);
    const int startRow = std::max(0, static_cast<int>(std::floor(visibleWorld.y / TerrainMap::TileSize)) - 1);
    const int endCol = std::min(TerrainMap::Columns - 1,
                                static_cast<int>(std::ceil((visibleWorld.x + visibleWorld.width) / TerrainMap::TileSize)) + 1);
    const int endRow = std::min(TerrainMap::Rows - 1,
                                static_cast<int>(std::ceil((visibleWorld.y + visibleWorld.height) / TerrainMap::TileSize)) + 1);

    auto sameColor = [](Color a, Color b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    };
    auto quantize = [](Color color) {
        color.r = static_cast<unsigned char>((color.r / 8) * 8);
        color.g = static_cast<unsigned char>((color.g / 8) * 8);
        color.b = static_cast<unsigned char>((color.b / 8) * 8);
        return color;
    };
    for (int row = startRow; row <= endRow; ++row) {
        int runStart = startCol;
        Color runColor{0, 0, 0, 0};
        bool hasRun = false;
        for (int col = startCol; col <= endCol; ++col) {
            const TerrainTile& tile = TerrainAt(map, col, row);
            Color color = ApplyTerrainShade(TerrainBiomeColor(tile.biome), tile);
            if ((tile.resource == TerrainResource::Wood || tile.resource == TerrainResource::FertileSoil) && tile.maxReserve > 0.01f) {
                const float fullness = TerrainClamp01(tile.reserve / tile.maxReserve);
                if (tile.resource == TerrainResource::Wood) {
                    color = Color{
                        static_cast<unsigned char>(std::clamp(static_cast<int>(color.r + (1.0f - fullness) * 64.0f), 0, 255)),
                        static_cast<unsigned char>(std::clamp(static_cast<int>(color.g - (1.0f - fullness) * 48.0f), 0, 255)),
                        static_cast<unsigned char>(std::clamp(static_cast<int>(color.b + (1.0f - fullness) * 20.0f), 0, 255)),
                        255
                    };
                } else {
                    color = Color{
                        static_cast<unsigned char>(std::clamp(static_cast<int>(color.r + fullness * 34.0f), 0, 255)),
                        static_cast<unsigned char>(std::clamp(static_cast<int>(color.g + fullness * 18.0f), 0, 255)),
                        static_cast<unsigned char>(std::clamp(static_cast<int>(color.b - fullness * 36.0f), 0, 255)),
                        255
                    };
                }
            }
            if (tile.river) color = TerrainResourceColor(TerrainResource::Water);
            color = quantize(color);
            if (!hasRun) {
                runStart = col;
                runColor = color;
                hasRun = true;
            } else if (!sameColor(runColor, color)) {
                DrawRectangle(runStart * TerrainMap::TileSize, row * TerrainMap::TileSize,
                              (col - runStart) * TerrainMap::TileSize, TerrainMap::TileSize, runColor);
                runStart = col;
                runColor = color;
            }
        }
        if (hasRun) {
            DrawRectangle(runStart * TerrainMap::TileSize, row * TerrainMap::TileSize,
                          (endCol - runStart + 1) * TerrainMap::TileSize, TerrainMap::TileSize, runColor);
        }
    }

    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            const TerrainTile& tile = TerrainAt(map, col, row);
            if (tile.resource == TerrainResource::None || tile.resource == TerrainResource::Water ||
                tile.resource == TerrainResource::FertileSoil || tile.reserve <= 0.0f) {
                continue;
            }
            const float fullness = tile.maxReserve > 0.01f ? TerrainClamp01(tile.reserve / tile.maxReserve) : 1.0f;
            const int sparseModulo = tile.resource == TerrainResource::Wood
                ? std::max(4, static_cast<int>(12.0f - fullness * 8.0f))
                : std::max(5, static_cast<int>(13.0f - fullness * 7.0f));
            const bool sparse = ((col * 19 + row * 31) % sparseModulo) == 0;
            if (!sparse) continue;
            const Vector2 p{col * TerrainMap::TileSize + TerrainMap::TileSize * 0.5f,
                            row * TerrainMap::TileSize + TerrainMap::TileSize * 0.5f};
            const Color resourceColor = TerrainResourceColor(tile.resource);
            if (tile.resource == TerrainResource::Wood) {
                DrawRectangle(static_cast<int>(p.x - 2), static_cast<int>(p.y - 3), 4, 5, Color{39, 99, 44, 255});
                DrawRectangle(static_cast<int>(p.x - 1), static_cast<int>(p.y + 1), 2, 3, Color{93, 69, 43, 255});
            } else {
                DrawRectangle(static_cast<int>(p.x - 2), static_cast<int>(p.y - 2), 5, 5, resourceColor);
                DrawRectangleLines(static_cast<int>(p.x - 2), static_cast<int>(p.y - 2), 5, 5, Fade(BLACK, 0.18f));
            }
        }
    }
}

} // namespace town
