#pragma once

#include "raylib.h"
#include "terrain.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace town {

enum class TileType {
    Empty,
    Road,
    Residence,
    Farm,
    Factory,
    MarketStall,
    Well,
    Plaza,
    Mine,
    Lumberyard,
    Bank
};

enum class BuildingKind {
    RichHome,
    MiddleHome,
    CommonHome,
    Farm,
    Workshop,
    Factory,
    MarketStall,
    Well,
    Mine,
    Lumberyard,
    Plaza,
    Bank,
    Church
};

struct TownTile {
    TileType type{TileType::Empty};
};

struct TownBuilding {
    BuildingKind kind{};
    std::string id;
    std::string label;
    Rectangle rect{};
    Vector2 door{};
    Color wall{};
    Color roof{};
    float rotation{};
};

struct TownPath {
    std::vector<Vector2> points;
};

struct TownMap {
    static constexpr int CellSize = 32;
    static constexpr int Columns = 40;
    static constexpr int Rows = 30;
    static constexpr int PixelWidth = TerrainMap::PixelWidth;
    static constexpr int PixelHeight = TerrainMap::PixelHeight;

    std::array<TownTile, Columns * Rows> tiles{};
    std::vector<TownBuilding> buildings;
    std::vector<Vector2> roadNodes;
    std::vector<Vector2> marketSpots;
    std::vector<TownBuilding> banks;
};

inline int TileIndex(int col, int row) {
    return row * TownMap::Columns + col;
}

inline void MarkTile(TownMap& map, int col, int row, TileType type) {
    if (col >= 0 && col < TownMap::Columns && row >= 0 && row < TownMap::Rows) {
        map.tiles[TileIndex(col, row)].type = type;
    }
}

inline void MarkRect(TownMap& map, int col, int row, int width, int height, TileType type) {
    for (int y = row; y < row + height; ++y) {
        for (int x = col; x < col + width; ++x) {
            MarkTile(map, x, y, type);
        }
    }
}

inline Vector2 CellCenter(int col, int row) {
    return Vector2{col * TownMap::CellSize + TownMap::CellSize * 0.5f,
                   row * TownMap::CellSize + TownMap::CellSize * 0.5f};
}

inline float Clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

inline Vector2 LerpVec(Vector2 a, Vector2 b, float t) {
    return Vector2{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

inline Vector2 BezierPoint(Vector2 a, Vector2 b, Vector2 c, Vector2 d, float t) {
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;
    return Vector2{
        uu * u * a.x + 3.0f * uu * t * b.x + 3.0f * u * tt * c.x + tt * t * d.x,
        uu * u * a.y + 3.0f * uu * t * b.y + 3.0f * u * tt * c.y + tt * t * d.y
    };
}

inline Vector2 RoadNormal(Vector2 a, Vector2 b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
    return Vector2{-dy / len, dx / len};
}

inline void AddRoadSample(TownMap& map, Vector2 p) {
    p.x = std::clamp(p.x, 10.0f, static_cast<float>(TownMap::PixelWidth - 18));
    p.y = std::clamp(p.y, 10.0f, static_cast<float>(TownMap::PixelHeight - 18));
    map.roadNodes.push_back(p);
    const int col = static_cast<int>(p.x) / TownMap::CellSize;
    const int row = static_cast<int>(p.y) / TownMap::CellSize;
    for (int yy = -1; yy <= 1; ++yy) {
        for (int xx = -1; xx <= 1; ++xx) {
            MarkTile(map, col + xx, row + yy, TileType::Road);
        }
    }
}

inline void AddBezierRoad(TownMap& map, Vector2 a, Vector2 b, Vector2 c, Vector2 d, int samples) {
    for (int i = 0; i <= samples; ++i) {
        AddRoadSample(map, BezierPoint(a, b, c, d, static_cast<float>(i) / static_cast<float>(samples)));
    }
}

inline Vector2 RoadOffsetPoint(const std::vector<Vector2>& path, float t, float offset) {
    const int segmentCount = static_cast<int>(path.size()) - 1;
    const float scaled = Clamp01(t) * static_cast<float>(segmentCount);
    const int seg = std::min(segmentCount - 1, std::max(0, static_cast<int>(scaled)));
    const float local = scaled - static_cast<float>(seg);
    const Vector2 center = LerpVec(path[seg], path[seg + 1], local);
    const Vector2 n = RoadNormal(path[seg], path[seg + 1]);
    return Vector2{center.x + n.x * offset, center.y + n.y * offset};
}

inline float RoadAngle(const std::vector<Vector2>& path, float t, float extra = 0.0f) {
    const int segmentCount = static_cast<int>(path.size()) - 1;
    const int seg = std::min(segmentCount - 1, std::max(0, static_cast<int>(Clamp01(t) * static_cast<float>(segmentCount))));
    const float angle = std::atan2(path[seg + 1].y - path[seg].y, path[seg + 1].x - path[seg].x) * 57.29578f;
    return angle + extra;
}

inline void AddBuilding(TownMap& map, BuildingKind kind, const char* id, const char* label, Rectangle rect,
                        Color wall, Color roof, TileType tileType, float rotation = 0.0f) {
    TownBuilding building;
    building.kind = kind;
    building.id = id;
    building.label = label;
    building.rect = rect;
    building.door = Vector2{rect.x + rect.width * 0.5f, rect.y + rect.height + 8.0f};
    building.wall = wall;
    building.roof = roof;
    building.rotation = rotation;
    map.buildings.push_back(building);
    MarkRect(map, static_cast<int>(rect.x) / TownMap::CellSize,
             static_cast<int>(rect.y) / TownMap::CellSize,
             std::max(1, static_cast<int>(std::ceil(rect.width / TownMap::CellSize))),
             std::max(1, static_cast<int>(std::ceil(rect.height / TownMap::CellSize))), tileType);
}

inline TownMap CreateTownMap() {
    TownMap map;

    MarkRect(map, 10, 8, 8, 5, TileType::Plaza);

    const std::vector<Vector2> highStreet = {{44, 360}, {164, 314}, {306, 340}, {440, 352}, {576, 344}, {716, 382}, {844, 338}};
    const std::vector<Vector2> churchRoad = {{438, 352}, {466, 272}, {508, 198}, {552, 118}, {620, 68}};
    const std::vector<Vector2> manorRoad = {{70, 112}, {164, 92}, {254, 128}, {334, 226}, {406, 332}};
    const std::vector<Vector2> craftRoad = {{610, 84}, {698, 122}, {758, 226}, {706, 338}, {670, 478}, {764, 632}};
    const std::vector<Vector2> millRoad = {{112, 642}, {214, 558}, {330, 524}, {442, 442}, {552, 452}, {690, 532}, {812, 650}};
    const std::vector<Vector2> farmRoad = {{58, 284}, {126, 330}, {202, 392}, {246, 474}, {190, 594}};
    const auto addPolylineRoad = [&map](const std::vector<Vector2>& path) {
        for (int i = 0; i + 1 < static_cast<int>(path.size()); ++i) {
            const Vector2 a = path[i];
            const Vector2 d = path[i + 1];
            const Vector2 dir{d.x - a.x, d.y - a.y};
            const Vector2 n = RoadNormal(a, d);
            const float curve = (i % 2 == 0 ? 34.0f : -28.0f);
            AddBezierRoad(map, a, Vector2{a.x + dir.x * 0.35f + n.x * curve, a.y + dir.y * 0.35f + n.y * curve},
                          Vector2{a.x + dir.x * 0.68f - n.x * curve * 0.55f, a.y + dir.y * 0.68f - n.y * curve * 0.55f},
                          d, 18);
        }
    };
    addPolylineRoad(highStreet);
    addPolylineRoad(churchRoad);
    addPolylineRoad(manorRoad);
    addPolylineRoad(craftRoad);
    addPolylineRoad(millRoad);
    addPolylineRoad(farmRoad);

    const Color richWall{226, 188, 126, 255};
    const Color middleWall{196, 151, 104, 255};
    const Color commonWall{166, 126, 87, 255};
    const Color redRoof{150, 56, 45, 255};
    const Color clayRoof{172, 91, 55, 255};
    const Color brownRoof{103, 72, 48, 255};
    const Color grayRoof{96, 96, 94, 255};

    for (int i = 0; i < 12; ++i) {
        const float t = 0.06f + static_cast<float>(i) * 0.078f + std::sin(static_cast<float>(i) * 1.7f) * 0.012f;
        const float offset = (i % 2 == 0 ? -58.0f : 54.0f) + std::sin(static_cast<float>(i) * 2.1f) * 9.0f;
        const Vector2 p = RoadOffsetPoint(manorRoad, t, offset);
        const float w = 46.0f + static_cast<float>(i % 3) * 4.0f;
        const float h = 34.0f + static_cast<float>((i + 1) % 3) * 3.0f;
        AddBuilding(map, BuildingKind::RichHome, ("R" + std::to_string(i + 1)).c_str(), "Villa",
                    Rectangle{p.x - w * 0.5f, p.y - h * 0.5f, w, h}, richWall, i % 3 == 0 ? grayRoof : redRoof,
                    TileType::Residence, RoadAngle(manorRoad, t, static_cast<float>((i % 5) - 2) * 2.0f));
    }

    for (int i = 0; i < 28; ++i) {
        const float t = 0.05f + static_cast<float>(i % 14) * 0.066f + std::sin(static_cast<float>(i) * 1.13f) * 0.012f;
        const float offset = (i < 14 ? -49.0f : 47.0f) + std::cos(static_cast<float>(i) * 1.41f) * 10.0f;
        const Vector2 p = RoadOffsetPoint(millRoad, t, offset);
        const float w = 32.0f + static_cast<float>(i % 4) * 2.0f;
        const float h = 27.0f + static_cast<float>((i + 2) % 3) * 2.0f;
        AddBuilding(map, BuildingKind::MiddleHome, ("M" + std::to_string(i + 1)).c_str(), "Home",
                    Rectangle{p.x - w * 0.5f, p.y - h * 0.5f, w, h}, middleWall, i % 4 == 0 ? redRoof : clayRoof,
                    TileType::Residence, RoadAngle(millRoad, t, static_cast<float>((i % 5) - 2) * 2.3f));
    }

    for (int i = 0; i < 40; ++i) {
        const float t = 0.04f + static_cast<float>(i % 20) * 0.047f + std::sin(static_cast<float>(i) * 0.91f) * 0.011f;
        const float offset = (i < 20 ? -42.0f : 45.0f) + std::cos(static_cast<float>(i) * 2.03f) * 8.0f;
        const Vector2 p = RoadOffsetPoint(craftRoad, t, offset);
        const float w = 29.0f + static_cast<float>(i % 3) * 2.0f;
        const float h = 23.0f + static_cast<float>((i + 1) % 3) * 2.0f;
        AddBuilding(map, BuildingKind::CommonHome, ("P" + std::to_string(i + 1)).c_str(), "Cottage",
                    Rectangle{p.x - w * 0.5f, p.y - h * 0.5f, w, h}, commonWall, i % 5 == 0 ? clayRoof : brownRoof,
                    TileType::Residence, RoadAngle(craftRoad, t, static_cast<float>((i % 5) - 2) * 2.0f));
    }

    for (int i = 0; i < 6; ++i) {
        const float x = 44.0f + static_cast<float>(i % 2) * 128.0f + std::sin(static_cast<float>(i)) * 12.0f;
        const float y = 302.0f + static_cast<float>(i / 2) * 84.0f + std::cos(static_cast<float>(i) * 1.4f) * 10.0f;
        AddBuilding(map, BuildingKind::Farm, ("farm_" + std::to_string(i)).c_str(), "Field",
                    Rectangle{x, y, 92.0f, 58.0f}, Color{145, 108, 63, 255}, Color{90, 151, 70, 255}, TileType::Farm,
                    static_cast<float>((i % 3) - 1) * 3.0f);
    }
    AddBuilding(map, BuildingKind::Well, "well", "Well", Rectangle{246.0f, 390.0f, 30.0f, 30.0f},
                Color{116, 137, 148, 255}, Color{89, 89, 85, 255}, TileType::Well);
    AddBuilding(map, BuildingKind::Church, "church", "St. Zeno", Rectangle{540.0f, 78.0f, 86.0f, 130.0f},
                Color{165, 160, 149, 255}, Color{86, 88, 96, 255}, TileType::Factory, -3.0f);

    AddBuilding(map, BuildingKind::Workshop, "blacksmith", "Smith", Rectangle{654.0f, 144.0f, 58.0f, 42.0f},
                Color{154, 130, 102, 255}, grayRoof, TileType::Factory, 4.0f);
    AddBuilding(map, BuildingKind::Workshop, "carpenter", "Wood", Rectangle{730.0f, 198.0f, 58.0f, 42.0f},
                Color{166, 121, 80, 255}, brownRoof, TileType::Factory, -4.0f);
    AddBuilding(map, BuildingKind::Workshop, "weaver", "Loom", Rectangle{640.0f, 262.0f, 58.0f, 42.0f},
                Color{158, 123, 170, 255}, Color{91, 71, 130, 255}, TileType::Factory, 3.0f);
    AddBuilding(map, BuildingKind::Workshop, "winery", "Winery", Rectangle{566.0f, 500.0f, 58.0f, 42.0f},
                Color{166, 156, 133, 255}, Color{117, 71, 58, 255}, TileType::Factory, 2.0f);
    AddBuilding(map, BuildingKind::Workshop, "stable", "Stable", Rectangle{704.0f, 606.0f, 62.0f, 40.0f},
                Color{142, 92, 54, 255}, Color{109, 74, 45, 255}, TileType::Factory, -3.0f);

    AddBuilding(map, BuildingKind::Factory, "factory_food", "Bakery", Rectangle{638.0f, 508.0f, 60.0f, 44.0f},
                Color{184, 151, 111, 255}, grayRoof, TileType::Factory, -3.0f);
    AddBuilding(map, BuildingKind::Factory, "factory_tools", "Tools", Rectangle{724.0f, 560.0f, 62.0f, 44.0f},
                Color{148, 136, 124, 255}, grayRoof, TileType::Factory, 5.0f);

    AddBuilding(map, BuildingKind::Mine, "mine_0", "Mine", Rectangle{44.0f, 626.0f, 70.0f, 44.0f},
                Color{117, 119, 121, 255}, grayRoof, TileType::Mine);
    AddBuilding(map, BuildingKind::Mine, "mine_1", "Mine", Rectangle{750.0f, 42.0f, 70.0f, 44.0f},
                Color{117, 119, 121, 255}, grayRoof, TileType::Mine);
    AddBuilding(map, BuildingKind::Lumberyard, "lumber", "Lumber", Rectangle{44.0f, 204.0f, 70.0f, 42.0f},
                Color{133, 96, 58, 255}, Color{68, 124, 72, 255}, TileType::Lumberyard);

    map.banks.push_back(TownBuilding{BuildingKind::Bank, "bank_river", "River Bank",
                                     Rectangle{266.0f, 214.0f, 78.0f, 48.0f},
                                     Vector2{305.0f, 270.0f},
                                     Color{154, 150, 139, 255}, Color{94, 88, 82, 255}, -2.0f});
    map.banks.push_back(TownBuilding{BuildingKind::Bank, "bank_market", "Market Bank",
                                     Rectangle{536.0f, 214.0f, 82.0f, 48.0f},
                                     Vector2{577.0f, 270.0f},
                                     Color{164, 158, 145, 255}, Color{90, 85, 80, 255}, 3.0f});
    map.banks.push_back(TownBuilding{BuildingKind::Bank, "bank_guild", "Guild Bank",
                                     Rectangle{394.0f, 450.0f, 88.0f, 50.0f},
                                     Vector2{438.0f, 508.0f},
                                     Color{150, 147, 138, 255}, Color{86, 82, 77, 255}, -4.0f});

    const std::array<Vector2, 22> stallCenters = {{
        {344, 270}, {392, 270}, {440, 270}, {488, 270}, {536, 270},
        {570, 310}, {570, 354}, {570, 398},
        {536, 438}, {488, 438}, {440, 438}, {392, 438}, {344, 438},
        {310, 398}, {310, 354}, {310, 310},
        {358, 314}, {420, 314}, {482, 314},
        {358, 394}, {420, 394}, {482, 394}
    }};
    for (int i = 0; i < static_cast<int>(stallCenters.size()); ++i) {
        Vector2 c = stallCenters[i];
        map.marketSpots.push_back(c);
    }

    return map;
}

inline const TownMap& GetTownMap() {
    static const TownMap map = CreateTownMap();
    return map;
}

inline float Distance(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline Vector2 NearestRoadPoint(const TownMap& map, Vector2 p) {
    Vector2 best = p;
    float bestDistance = 1000000.0f;
    for (Vector2 node : map.roadNodes) {
        const float d = Distance(p, node);
        if (d < bestDistance) {
            bestDistance = d;
            best = node;
        }
    }
    return best;
}

inline TownPath BuildPath(const TownMap& map, Vector2 from, Vector2 to) {
    const Vector2 startRoad = NearestRoadPoint(map, from);
    const Vector2 endRoad = NearestRoadPoint(map, to);
    const Vector2 hub{440.0f, 360.0f};
    TownPath path;
    path.points.push_back(from);
    path.points.push_back(startRoad);
    if (Distance(startRoad, endRoad) > 72.0f) {
        path.points.push_back(Vector2{startRoad.x, hub.y});
        path.points.push_back(hub);
        path.points.push_back(Vector2{endRoad.x, hub.y});
    }
    path.points.push_back(endRoad);
    path.points.push_back(to);
    std::vector<Vector2> compact;
    for (Vector2 point : path.points) {
        if (compact.empty() || Distance(compact.back(), point) > 3.0f) {
            compact.push_back(point);
        }
    }
    path.points = compact;
    return path;
}

inline std::vector<const TownBuilding*> ResidenceBuildings(const TownMap& map) {
    std::vector<const TownBuilding*> homes;
    for (const auto& building : map.buildings) {
        if (building.kind == BuildingKind::RichHome || building.kind == BuildingKind::MiddleHome ||
            building.kind == BuildingKind::CommonHome) {
            homes.push_back(&building);
        }
    }
    return homes;
}

inline std::vector<Vector2> HomeDoors(const TownMap& map) {
    std::vector<Vector2> homes;
    for (const TownBuilding* building : ResidenceBuildings(map)) {
        homes.push_back(building->door);
    }
    return homes;
}

inline std::vector<Vector2> WorkplaceDoors(const TownMap& map, BuildingKind kind) {
    std::vector<Vector2> doors;
    for (const auto& building : map.buildings) {
        if (building.kind == kind) {
            doors.push_back(building.door);
        }
    }
    return doors;
}

inline Vector2 PlazaCenter() {
    return Vector2{440.0f, 360.0f};
}

inline Vector2 MarketSpotForResident(const TownMap& map, int residentId) {
    if (map.marketSpots.empty()) {
        return PlazaCenter();
    }
    return map.marketSpots[residentId % map.marketSpots.size()];
}

inline Vector2 WorkplaceForProfession(const TownMap& map, int residentId, int employerCompany, int factoryCount,
                                      const std::string& profession) {
    if (employerCompany >= 0 && factoryCount > 0) {
        std::vector<Vector2> factories = WorkplaceDoors(map, BuildingKind::Factory);
        std::vector<Vector2> workshops = WorkplaceDoors(map, BuildingKind::Workshop);
        factories.insert(factories.end(), workshops.begin(), workshops.end());
        if (!factories.empty()) {
            return factories[employerCompany % factories.size()];
        }
    }

    BuildingKind kind = BuildingKind::Workshop;
    if (profession == "Farmer") kind = BuildingKind::Farm;
    else if (profession == "Miner") kind = BuildingKind::Mine;
    else if (profession == "Carpenter") kind = BuildingKind::Lumberyard;
    else if (profession == "Merchant" || profession == "Hauler") return MarketSpotForResident(map, residentId);

    std::vector<Vector2> doors = WorkplaceDoors(map, kind);
    if (doors.empty()) {
        return PlazaCenter();
    }
    return doors[residentId % doors.size()];
}

inline void DrawRoofTrapezoid(Rectangle body, Color roof) {
    const Vector2 a{body.x - 5.0f, body.y};
    const Vector2 b{body.x + 4.0f, body.y - 17.0f};
    const Vector2 c{body.x + body.width - 4.0f, body.y - 17.0f};
    const Vector2 d{body.x + body.width + 5.0f, body.y};
    DrawTriangle(a, b, c, roof);
    DrawTriangle(a, c, d, roof);
    DrawLineEx(b, c, 2.0f, Fade(BLACK, 0.18f));
}

inline unsigned int StableHash(const std::string& text) {
    unsigned int hash = 2166136261u;
    for (char ch : text) {
        hash = (hash ^ static_cast<unsigned int>(ch)) * 16777619u;
    }
    return hash;
}

inline Vector2 RectCenter(Rectangle r) {
    return Vector2{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
}

inline Vector2 RotateAround(Vector2 p, Vector2 origin, float degrees) {
    const float radians = degrees * 0.0174532925f;
    const float s = std::sin(radians);
    const float c = std::cos(radians);
    const float x = p.x - origin.x;
    const float y = p.y - origin.y;
    return Vector2{origin.x + x * c - y * s, origin.y + x * s + y * c};
}

inline void DrawRotatedQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Vector2 origin, float degrees, Color color) {
    a = RotateAround(a, origin, degrees);
    b = RotateAround(b, origin, degrees);
    c = RotateAround(c, origin, degrees);
    d = RotateAround(d, origin, degrees);
    DrawTriangle(a, b, c, color);
    DrawTriangle(a, c, d, color);
}

inline void DrawRotatedLine(Vector2 a, Vector2 b, Vector2 origin, float degrees, float thick, Color color) {
    DrawLineEx(RotateAround(a, origin, degrees), RotateAround(b, origin, degrees), thick, color);
}

inline void DrawSmoke(Vector2 origin, unsigned int seed) {
    const float phase = static_cast<float>(GetTime()) * 0.8f + static_cast<float>(seed % 97) * 0.1f;
    for (int i = 0; i < 4; ++i) {
        const float rise = std::fmod(phase * 9.0f + static_cast<float>(i) * 9.0f, 36.0f);
        const float drift = std::sin(phase + static_cast<float>(i)) * 3.0f;
        DrawCircleV(Vector2{origin.x + drift, origin.y - rise}, 4.0f + static_cast<float>(i), Fade(Color{118, 118, 112, 255}, 0.24f));
    }
}

inline Color ColorLerp(Color a, Color b, float t) {
    t = Clamp01(t);
    return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t),
                 static_cast<unsigned char>(a.g + (b.g - a.g) * t),
                 static_cast<unsigned char>(a.b + (b.b - a.b) * t),
                 static_cast<unsigned char>(a.a + (b.a - a.a) * t)};
}

inline void DrawWindowCross(Rectangle w, Color frame) {
    DrawRectangleRec(w, Color{231, 200, 96, 255});
    DrawRectangleLinesEx(w, 1.0f, frame);
    DrawLine(static_cast<int>(w.x + w.width * 0.5f), static_cast<int>(w.y),
             static_cast<int>(w.x + w.width * 0.5f), static_cast<int>(w.y + w.height), frame);
    DrawLine(static_cast<int>(w.x), static_cast<int>(w.y + w.height * 0.5f),
             static_cast<int>(w.x + w.width), static_cast<int>(w.y + w.height * 0.5f), frame);
}

inline void DrawTreeCluster(Vector2 center, int count, float radius, unsigned int seed) {
    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>((i * 137 + seed) % 360) * 0.0174532925f;
        const float dist = radius * (0.25f + static_cast<float>((i * 41 + seed) % 100) / 135.0f);
        const Vector2 p{center.x + std::cos(angle) * dist, center.y + std::sin(angle) * dist * 0.72f};
        DrawRectangle(static_cast<int>(p.x - 3), static_cast<int>(p.y + 5), 6, 16, Color{87, 61, 38, 255});
        DrawCircleV(Vector2{p.x - 7.0f, p.y}, 10.0f, Color{45, 96, 54, 255});
        DrawCircleV(Vector2{p.x + 7.0f, p.y + 1.0f}, 11.0f, Color{63, 121, 61, 255});
        DrawCircleV(Vector2{p.x, p.y - 8.0f}, 12.0f, Color{38, 88, 49, 255});
    }
}

inline void DrawRiver() {
    const Vector2 a{-32.0f, 62.0f};
    const Vector2 b{210.0f, 102.0f};
    const Vector2 c{208.0f, 352.0f};
    const Vector2 d{438.0f, 402.0f};
    const Vector2 e{650.0f, 454.0f};
    const Vector2 f{660.0f, 650.0f};
    const Vector2 g{920.0f, 742.0f};
    const float phase = static_cast<float>(GetTime()) * 2.2f;
    Vector2 prev = a;
    for (int i = 1; i <= 70; ++i) {
        const float t = static_cast<float>(i) / 70.0f;
        const Vector2 p = BezierPoint(a, b, c, d, t);
        DrawLineEx(prev, p, 39.0f, Color{57, 126, 177, 255});
        DrawLineEx(prev, p, 28.0f, Color{74, 153, 201, 255});
        prev = p;
    }
    prev = d;
    for (int i = 1; i <= 70; ++i) {
        const float t = static_cast<float>(i) / 70.0f;
        const Vector2 p = BezierPoint(d, e, f, g, t);
        DrawLineEx(prev, p, 41.0f, Color{55, 123, 174, 255});
        DrawLineEx(prev, p, 29.0f, Color{77, 158, 204, 255});
        prev = p;
    }
    prev = a;
    for (int i = 1; i <= 56; ++i) {
        const float t = static_cast<float>(i) / 56.0f;
        const Vector2 p = BezierPoint(a, b, c, d, t);
        const float off = std::sin(t * 30.0f + phase) * 7.0f;
        DrawLineEx(Vector2{prev.x, prev.y + off}, Vector2{p.x, p.y + std::sin(t * 30.0f + phase) * 7.0f},
                   1.7f, Fade(WHITE, 0.32f));
        prev = p;
    }
    prev = d;
    for (int i = 1; i <= 56; ++i) {
        const float t = static_cast<float>(i) / 56.0f;
        const Vector2 p = BezierPoint(d, e, f, g, t);
        DrawLineEx(Vector2{prev.x, prev.y + std::sin(t * 28.0f + phase + 1.1f) * 7.0f},
                   Vector2{p.x, p.y + std::sin(t * 28.0f + phase + 1.1f) * 7.0f},
                   1.7f, Fade(WHITE, 0.30f));
        prev = p;
    }
}

inline void DrawNaturalResourceAreas() {
    DrawTreeCluster(Vector2{70, 96}, 18, 62.0f, 11);
    DrawTreeCluster(Vector2{822, 168}, 20, 74.0f, 23);
    DrawTreeCluster(Vector2{804, 642}, 22, 82.0f, 37);

    const std::array<Rectangle, 5> fields = {{{36, 302, 92, 58}, {156, 384, 92, 58}, {48, 470, 98, 58}, {142, 568, 96, 54}, {238, 486, 86, 50}}};
    for (int i = 0; i < static_cast<int>(fields.size()); ++i) {
        const Rectangle r = fields[i];
        DrawRectangleRec(r, Color{112, 139, 62, 255});
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 6; ++x) {
                const Color crop = ((x + y + i) % 2 == 0) ? Color{218, 178, 65, 255} : Color{92, 153, 68, 255};
                DrawRectangle(static_cast<int>(r.x + 5 + x * 14), static_cast<int>(r.y + 5 + y * 12), 12, 10, crop);
            }
        }
        DrawRectangleLinesEx(r, 1.0f, Color{107, 84, 44, 255});
    }

    DrawRectangleRec(Rectangle{704, 604, 126, 74}, Color{145, 197, 116, 255});
    for (int i = 0; i < 9; ++i) {
        const float x = 720.0f + static_cast<float>((i * 31) % 92);
        const float y = 622.0f + static_cast<float>((i * 17) % 38);
        DrawCircleV(Vector2{x, y}, 5.0f, Color{244, 244, 232, 255});
        DrawCircleV(Vector2{x + 5.0f, y - 2.0f}, 3.0f, Color{244, 244, 232, 255});
        DrawCircleV(Vector2{x + 8.0f, y - 4.0f}, 1.5f, Color{42, 42, 40, 255});
    }
}

inline void DrawWindmill(Vector2 hub, float radius, Color blade) {
    const float angle = static_cast<float>(GetTime()) * 85.0f;
    DrawCircleV(hub, 5.0f, Color{83, 61, 42, 255});
    for (int i = 0; i < 4; ++i) {
        const float a = (angle + i * 90.0f) * 0.0174532925f;
        const Vector2 tip{hub.x + std::cos(a) * radius, hub.y + std::sin(a) * radius};
        DrawLineEx(hub, tip, 3.0f, blade);
        DrawCircleV(tip, 4.0f, Fade(blade, 0.85f));
    }
}

inline void DrawSpecialWorkshop(const TownBuilding& building) {
    const Rectangle r = building.rect;
    const unsigned int seed = StableHash(building.id);
    DrawRectangleRec(r, building.id == "blacksmith" ? Color{82, 79, 75, 255} :
                        building.id == "weaver" ? Color{222, 211, 184, 255} :
                        building.id == "winery" ? Color{169, 161, 139, 255} :
                        building.id == "stable" ? Color{135, 84, 48, 255} :
                        building.id == "carpenter" ? Color{141, 94, 58, 255} : building.wall);
    DrawRoofTrapezoid(r, building.roof);
    for (int y = 0; y < 3; ++y) {
        DrawLine(static_cast<int>(r.x + 4), static_cast<int>(r.y + 9 + y * 10),
                 static_cast<int>(r.x + r.width - 4), static_cast<int>(r.y + 9 + y * 10), Fade(BLACK, 0.25f));
    }
    DrawRectangle(static_cast<int>(r.x + r.width * 0.45f), static_cast<int>(r.y + r.height - 18), 13, 18, Color{73, 50, 35, 255});
    if (building.id == "blacksmith") {
        DrawRectangle(static_cast<int>(r.x + 6), static_cast<int>(r.y - 22), 12, 24, Color{54, 50, 47, 255});
        DrawSmoke(Vector2{r.x + 12.0f, r.y - 24.0f}, seed);
        DrawRectangle(static_cast<int>(r.x + r.width - 16), static_cast<int>(r.y + r.height + 6), 18, 6, Color{64, 64, 62, 255});
        DrawRectangle(static_cast<int>(r.x + r.width - 10), static_cast<int>(r.y + r.height), 7, 8, Color{64, 64, 62, 255});
    } else if (building.id == "weaver") {
        const Color cloths[] = {Color{170, 71, 82, 255}, Color{238, 219, 126, 255}, Color{88, 126, 182, 255}};
        DrawLineEx(Vector2{r.x - 8, r.y + r.height + 7}, Vector2{r.x + r.width + 10, r.y + r.height + 4}, 2.0f, Color{83, 60, 42, 255});
        for (int i = 0; i < 3; ++i) {
            DrawRectangle(static_cast<int>(r.x + 5 + i * 17), static_cast<int>(r.y + r.height + 6), 12, 16, cloths[i]);
        }
    } else if (building.id == "carpenter") {
        for (int i = 0; i < 4; ++i) {
            DrawLineEx(Vector2{r.x + 6.0f + i * 12.0f, r.y + r.height + 8.0f},
                       Vector2{r.x + 24.0f + i * 12.0f, r.y + r.height + 3.0f}, 5.0f, Color{103, 68, 41, 255});
        }
    } else if (building.id == "winery") {
        for (int i = 0; i < 4; ++i) {
            DrawLineEx(Vector2{r.x + 7.0f + i * 11.0f, r.y + 5.0f}, Vector2{r.x + 11.0f + i * 11.0f, r.y + 27.0f},
                       2.0f, Color{53, 112, 56, 255});
            DrawCircleV(Vector2{r.x + 12.0f + i * 11.0f, r.y + 16.0f}, 3.0f, Color{88, 49, 118, 255});
        }
        for (int i = 0; i < 2; ++i) {
            DrawCircleV(Vector2{r.x + r.width + 9.0f + i * 12.0f, r.y + r.height - 3.0f}, 8.0f, Color{117, 73, 41, 255});
            DrawLineEx(Vector2{r.x + r.width + 1.0f + i * 12.0f, r.y + r.height - 3.0f},
                       Vector2{r.x + r.width + 17.0f + i * 12.0f, r.y + r.height - 3.0f}, 1.0f, Color{76, 47, 30, 255});
        }
    } else if (building.id == "stable") {
        for (int i = 0; i < 5; ++i) {
            DrawLineEx(Vector2{r.x - 10.0f + i * 16.0f, r.y + r.height + 3.0f},
                       Vector2{r.x - 10.0f + i * 16.0f, r.y + r.height + 27.0f}, 3.0f, Color{89, 58, 36, 255});
        }
        DrawLineEx(Vector2{r.x - 14.0f, r.y + r.height + 14.0f}, Vector2{r.x + r.width + 14.0f, r.y + r.height + 14.0f},
                   3.0f, Color{89, 58, 36, 255});
        DrawTriangle(Vector2{r.x + r.width - 8.0f, r.y + r.height + 12.0f},
                     Vector2{r.x + r.width + 14.0f, r.y + r.height + 7.0f},
                     Vector2{r.x + r.width + 12.0f, r.y + r.height + 22.0f}, Color{39, 34, 28, 255});
        DrawRectangle(static_cast<int>(r.x + r.width - 9), static_cast<int>(r.y + r.height + 11), 23, 12, Color{39, 34, 28, 255});
        DrawCircleV(Vector2{r.x + 10.0f, r.y + r.height + 12.0f}, 9.0f, Color{218, 181, 82, 255});
    }
    DrawWindowCross(Rectangle{r.x + 9, r.y + 13, 10, 10}, Color{68, 50, 37, 255});
}

inline void DrawSpecialFactory(const TownBuilding& building) {
    const Rectangle r = building.rect;
    const bool bakery = building.id == "factory_food";
    DrawRectangleRec(Rectangle{r.x, r.y + 10, r.width, r.height - 10}, bakery ? Color{215, 181, 109, 255} : Color{72, 69, 66, 255});
    DrawRectangleRec(Rectangle{r.x + 4, r.y + r.height - 14, r.width - 8, 14}, Color{116, 113, 105, 255});
    DrawRoofTrapezoid(r, bakery ? Color{134, 83, 51, 255} : Color{76, 76, 74, 255});
    if (bakery) {
        DrawWindmill(Vector2{r.x + r.width + 18.0f, r.y + 8.0f}, 22.0f, Color{232, 218, 174, 255});
        DrawLineEx(Vector2{r.x + r.width + 18.0f, r.y + 8.0f}, Vector2{r.x + r.width + 10.0f, r.y + r.height}, 3.0f, Color{95, 68, 43, 255});
        DrawRectangle(static_cast<int>(r.x + 9), static_cast<int>(r.y + 18), 18, 12, Color{245, 210, 104, 255});
    } else {
        DrawRectangle(static_cast<int>(r.x + 8), static_cast<int>(r.y - 18), 12, 28, Color{58, 56, 54, 255});
        DrawSmoke(Vector2{r.x + 14.0f, r.y - 20.0f}, StableHash(building.id));
        DrawRectangle(static_cast<int>(r.x + r.width + 5), static_cast<int>(r.y + r.height - 8), 18, 6, Color{61, 61, 60, 255});
        DrawRectangle(static_cast<int>(r.x + r.width + 11), static_cast<int>(r.y + r.height - 16), 7, 10, Color{61, 61, 60, 255});
    }
}

inline void DrawHouseDetails(const TownBuilding& building) {
    const Rectangle r = building.rect;
    const unsigned int seed = StableHash(building.id);
    const Vector2 o = RectCenter(r);
    const float rot = building.rotation;
    const Color timber{83, 50, 31, 255};
    const Color plaster = building.kind == BuildingKind::RichHome ? Color{239, 225, 190, 255}
                        : building.kind == BuildingKind::MiddleHome ? Color{228, 212, 175, 255}
                        : Color{216, 202, 171, 255};
    DrawRotatedQuad(Vector2{r.x, r.y}, Vector2{r.x + r.width, r.y},
                    Vector2{r.x + r.width, r.y + r.height}, Vector2{r.x, r.y + r.height}, o, rot, plaster);
    DrawRotatedQuad(Vector2{r.x + 5, r.y - 20}, Vector2{r.x + 13, r.y - 20},
                    Vector2{r.x + 13, r.y - 3}, Vector2{r.x + 5, r.y - 3}, o, rot, Color{85, 60, 45, 255});
    DrawSmoke(RotateAround(Vector2{r.x + 9.0f, r.y - 22.0f}, o, rot), seed);
    const Vector2 roofA = RotateAround(Vector2{r.x - 6.0f, r.y}, o, rot);
    const Vector2 roofB = RotateAround(Vector2{r.x + r.width * 0.5f - 5.0f, r.y - 18.0f - static_cast<float>(seed % 5)}, o, rot);
    const Vector2 roofC = RotateAround(Vector2{r.x + r.width * 0.5f + 5.0f, r.y - 17.0f}, o, rot);
    const Vector2 roofD = RotateAround(Vector2{r.x + r.width + 6.0f, r.y}, o, rot);
    DrawTriangle(roofA, roofB, roofC, building.roof);
    DrawTriangle(roofA, roofC, roofD, building.roof);
    for (int tile = 0; tile < 4; ++tile) {
        const float tx = r.x + 4.0f + tile * (r.width - 8.0f) / 4.0f;
        DrawRotatedLine(Vector2{tx, r.y - 3.0f}, Vector2{tx + 10.0f, r.y - 14.0f}, o, rot, 1.0f,
                        Fade(tile % 2 == 0 ? Color{99, 67, 55, 255} : Color{208, 113, 73, 255}, 0.45f));
    }
    DrawRotatedLine(Vector2{r.x + 2, r.y + 2}, Vector2{r.x + r.width - 2, r.y + 2}, o, rot, 3.0f, timber);
    DrawRotatedLine(Vector2{r.x + 2, r.y + r.height - 2}, Vector2{r.x + r.width - 2, r.y + r.height - 2}, o, rot, 3.0f, timber);
    DrawRotatedLine(Vector2{r.x + 4, r.y + 2}, Vector2{r.x + 4, r.y + r.height - 2}, o, rot, 3.0f, timber);
    DrawRotatedLine(Vector2{r.x + r.width - 4, r.y + 2}, Vector2{r.x + r.width - 4, r.y + r.height - 2}, o, rot, 3.0f, timber);
    DrawRotatedLine(Vector2{r.x + r.width * 0.5f, r.y + 2}, Vector2{r.x + r.width * 0.5f, r.y + r.height - 2}, o, rot, 2.3f, timber);
    DrawRotatedLine(Vector2{r.x + 5, r.y + 4}, Vector2{r.x + r.width * 0.5f, r.y + r.height - 4}, o, rot, 2.0f, timber);
    DrawRotatedLine(Vector2{r.x + r.width - 5, r.y + 4}, Vector2{r.x + r.width * 0.5f, r.y + r.height - 4}, o, rot, 2.0f, timber);
    DrawRotatedQuad(Vector2{r.x + r.width * 0.42f, r.y + r.height - 16.0f},
                    Vector2{r.x + r.width * 0.42f + 11.0f, r.y + r.height - 16.0f},
                    Vector2{r.x + r.width * 0.42f + 11.0f, r.y + r.height},
                    Vector2{r.x + r.width * 0.42f, r.y + r.height}, o, rot, Color{82, 54, 39, 255});
    const int windows = 2 + static_cast<int>(seed % 2u);
    for (int i = 0; i < windows; ++i) {
        const float wx = r.x + 7.0f + static_cast<float>(i) * ((r.width - 17.0f) / static_cast<float>(std::max(1, windows - 1)));
        DrawRotatedQuad(Vector2{wx, r.y + 9.0f}, Vector2{wx + 8, r.y + 9.0f}, Vector2{wx + 8, r.y + 17.0f}, Vector2{wx, r.y + 17.0f},
                        o, rot, Color{236, 205, 98, 255});
        DrawRotatedLine(Vector2{wx + 4, r.y + 9.0f}, Vector2{wx + 4, r.y + 17.0f}, o, rot, 1.0f, Color{94, 70, 47, 255});
        DrawRotatedLine(Vector2{wx, r.y + 13.0f}, Vector2{wx + 8, r.y + 13.0f}, o, rot, 1.0f, Color{94, 70, 47, 255});
        DrawRotatedLine(Vector2{wx, r.y + 9.0f}, Vector2{wx + 8, r.y + 17.0f}, o, rot, 0.8f, Fade(Color{94, 70, 47, 255}, 0.72f));
        DrawRotatedLine(Vector2{wx + 8, r.y + 9.0f}, Vector2{wx, r.y + 17.0f}, o, rot, 0.8f, Fade(Color{94, 70, 47, 255}, 0.72f));
    }
    DrawText(building.id.c_str(), static_cast<int>(r.x + 3), static_cast<int>(r.y + r.height + 3), 8, Color{43, 48, 48, 255});
}

inline void DrawGothicChurch(const TownBuilding& building) {
    const Rectangle r = building.rect;
    const float bellSwing = std::sin(static_cast<float>(GetTime()) * 2.6f) * 7.0f;
    DrawRectangle(static_cast<int>(r.x + 8), static_cast<int>(r.y + 12), static_cast<int>(r.width), static_cast<int>(r.height),
                  Fade(BLACK, 0.20f));
    DrawRectangleRec(Rectangle{r.x + 12, r.y + 44, r.width - 24, r.height - 44}, building.wall);
    DrawRectangleLinesEx(Rectangle{r.x + 12, r.y + 44, r.width - 24, r.height - 44}, 1.0f, Color{82, 82, 78, 255});
    DrawTriangle(Vector2{r.x + 8, r.y + 44}, Vector2{r.x + r.width * 0.5f, r.y + 8}, Vector2{r.x + r.width - 8, r.y + 44}, building.roof);
    DrawRectangle(static_cast<int>(r.x + r.width * 0.5f - 13), static_cast<int>(r.y + 18), 26, 94, Color{145, 142, 136, 255});
    DrawTriangle(Vector2{r.x + r.width * 0.5f - 20, r.y + 20}, Vector2{r.x + r.width * 0.5f, r.y - 38},
                 Vector2{r.x + r.width * 0.5f + 20, r.y + 20}, Color{61, 66, 76, 255});
    DrawCircleSector(Vector2{r.x + r.width * 0.5f, r.y + 82}, 16.0f, 180.0f, 360.0f, 18, Color{63, 57, 70, 255});
    DrawRectangle(static_cast<int>(r.x + r.width * 0.5f - 16), static_cast<int>(r.y + 82), 32, 38, Color{63, 57, 70, 255});
    const Color glass[] = {Color{191, 59, 70, 255}, Color{59, 104, 179, 255}, Color{229, 188, 68, 255}, Color{70, 147, 103, 255}};
    for (int i = 0; i < 4; ++i) {
        DrawRectangle(static_cast<int>(r.x + r.width * 0.5f - 13 + i * 7), static_cast<int>(r.y + 88), 6, 18, glass[i]);
    }
    DrawLineEx(Vector2{r.x + r.width * 0.5f, r.y + 48}, Vector2{r.x + r.width * 0.5f + bellSwing, r.y + 64}, 2.0f, Color{82, 64, 41, 255});
    DrawCircleV(Vector2{r.x + r.width * 0.5f + bellSwing, r.y + 67}, 5.0f, Color{184, 136, 53, 255});
    for (int i = 0; i < 8; ++i) {
        const float gx = r.x - 28.0f + static_cast<float>(i % 4) * 16.0f;
        const float gy = r.y + r.height - 22.0f + static_cast<float>(i / 4) * 14.0f;
        DrawLineEx(Vector2{gx, gy}, Vector2{gx + 8, gy - 10}, 2.0f, Color{116, 116, 112, 255});
        DrawLineEx(Vector2{gx + 4, gy - 5}, Vector2{gx + 12, gy + 1}, 1.0f, Color{116, 116, 112, 255});
    }
    DrawText(building.label.c_str(), static_cast<int>(r.x + 15), static_cast<int>(r.y + r.height + 4), 9, Color{43, 48, 48, 255});
}

inline void DrawTownBuilding(const TownBuilding& building) {
    if (building.kind == BuildingKind::Church) {
        DrawGothicChurch(building);
        return;
    }
    const Vector2 center = RectCenter(building.rect);
    DrawRotatedQuad(Vector2{building.rect.x + 5, building.rect.y + 6},
                    Vector2{building.rect.x + building.rect.width + 5, building.rect.y + 6},
                    Vector2{building.rect.x + building.rect.width + 5, building.rect.y + building.rect.height + 6},
                    Vector2{building.rect.x + 5, building.rect.y + building.rect.height + 6},
                    center, building.rotation, Fade(BLACK, 0.22f));
    if (building.kind == BuildingKind::Farm) {
        DrawRectangleRec(building.rect, building.wall);
        DrawRectangleLinesEx(building.rect, 1.0f, Color{104, 80, 49, 255});
        for (int row = 0; row < 6; ++row) {
            DrawLine(static_cast<int>(building.rect.x + 8), static_cast<int>(building.rect.y + 9 + row * 12),
                     static_cast<int>(building.rect.x + building.rect.width - 8), static_cast<int>(building.rect.y + 9 + row * 12),
                     row % 2 == 0 ? Color{93, 151, 70, 255} : Color{213, 189, 88, 255});
        }
        return;
    }
    if (building.kind == BuildingKind::Workshop) {
        DrawSpecialWorkshop(building);
        return;
    }
    if (building.kind == BuildingKind::Factory) {
        DrawSpecialFactory(building);
        return;
    }
    if (building.kind == BuildingKind::Well) {
        const float bucket = std::sin(static_cast<float>(GetTime()) * 2.2f) * 4.0f;
        DrawCircleV(Vector2{building.rect.x + building.rect.width * 0.5f, building.rect.y + building.rect.height * 0.5f},
                    building.rect.width * 0.48f, building.wall);
        DrawCircleV(Vector2{building.rect.x + building.rect.width * 0.5f, building.rect.y + building.rect.height * 0.5f},
                    building.rect.width * 0.30f, Color{91, 129, 160, 255});
        DrawRoofTrapezoid(Rectangle{building.rect.x - 2.0f, building.rect.y + 4.0f, building.rect.width + 4.0f, 10.0f}, building.roof);
        DrawLineEx(Vector2{building.rect.x + 6.0f, building.rect.y - 2.0f}, Vector2{building.rect.x + 24.0f, building.rect.y - 12.0f}, 3.0f, Color{72, 58, 42, 255});
        DrawLineEx(Vector2{building.rect.x + 25.0f, building.rect.y - 2.0f}, Vector2{building.rect.x + 25.0f, building.rect.y + 15.0f + bucket}, 1.0f, Color{61, 48, 36, 255});
        DrawRectangle(static_cast<int>(building.rect.x + 22), static_cast<int>(building.rect.y + 17 + bucket), 8, 11, Color{109, 74, 45, 255});
        Vector2 person{building.rect.x - 12.0f, building.rect.y + 30.0f};
        DrawCircleV(Vector2{person.x, person.y - 12.0f}, 3.5f, Color{204, 151, 102, 255});
        DrawRectangle(static_cast<int>(person.x - 3), static_cast<int>(person.y - 9), 7, 10, Color{68, 116, 151, 255});
        DrawLineEx(Vector2{person.x + 3.0f, person.y - 5.0f}, Vector2{building.rect.x + 2.0f, building.rect.y + 20.0f}, 1.2f, Color{69, 57, 48, 255});
        DrawLineEx(Vector2{person.x - 2.0f, person.y + 1.0f}, Vector2{person.x - 5.0f, person.y + 8.0f}, 1.4f, Color{45, 43, 42, 255});
        DrawLineEx(Vector2{person.x + 2.0f, person.y + 1.0f}, Vector2{person.x + 5.0f, person.y + 8.0f}, 1.4f, Color{45, 43, 42, 255});
        return;
    }
    if (building.kind == BuildingKind::Mine) {
        DrawRectangleRec(building.rect, Color{121, 119, 114, 255});
        DrawCircleV(Vector2{building.rect.x + 20.0f, building.rect.y + 18.0f}, 18.0f, Color{87, 88, 92, 255});
        DrawCircleV(Vector2{building.rect.x + 46.0f, building.rect.y + 27.0f}, 20.0f, Color{73, 75, 80, 255});
        DrawRectangle(static_cast<int>(building.rect.x + 22), static_cast<int>(building.rect.y + 19), 28, 25, Color{40, 39, 39, 255});
        return;
    }
    if (building.kind == BuildingKind::Lumberyard) {
        DrawRectangleRec(building.rect, Color{125, 94, 57, 255});
        for (int i = 0; i < 4; ++i) {
            DrawCircle(static_cast<int>(building.rect.x + 12 + i * 14), static_cast<int>(building.rect.y + 15), 8, Color{91, 128, 71, 255});
            DrawRectangle(static_cast<int>(building.rect.x + 9 + i * 14), static_cast<int>(building.rect.y + 21), 6, 17, Color{95, 66, 42, 255});
        }
        return;
    }

    if (building.kind == BuildingKind::MarketStall) {
        DrawRectangleRec(building.rect, building.wall);
        DrawRoofTrapezoid(building.rect, building.roof);
        return;
    }

    DrawHouseDetails(building);
    if (!building.label.empty()) {
        DrawText(building.label.c_str(), static_cast<int>(building.rect.x + 4), static_cast<int>(building.rect.y + building.rect.height + 13),
                 8, Color{43, 48, 48, 255});
    }
}

inline void DrawBankBuilding(const TownBuilding& building, bool alive) {
    Rectangle r = building.rect;
    const Color wall = alive ? building.wall : Color{126, 126, 122, 255};
    const Color roof = alive ? building.roof : Color{95, 95, 93, 255};
    DrawRectangle(static_cast<int>(r.x + 6), static_cast<int>(r.y + 7),
                  static_cast<int>(r.width), static_cast<int>(r.height), Fade(BLACK, 0.22f));
    DrawRoofTrapezoid(r, roof);
    DrawRectangleRec(r, wall);
    for (int y = 0; y < 4; ++y) {
        DrawLine(static_cast<int>(r.x + 4), static_cast<int>(r.y + 9 + y * 9),
                 static_cast<int>(r.x + r.width - 4), static_cast<int>(r.y + 9 + y * 9),
                 Fade(Color{72, 72, 68, 255}, 0.34f));
    }
    for (int i = 0; i < 3; ++i) {
        const float x = r.x + 12.0f + i * ((r.width - 24.0f) / 2.0f);
        DrawRectangle(static_cast<int>(x - 3), static_cast<int>(r.y + 15), 6, 25,
                      alive ? Color{216, 199, 161, 255} : Color{104, 104, 101, 255});
        DrawRectangle(static_cast<int>(x - 5), static_cast<int>(r.y + 39), 10, 4, Color{86, 78, 67, 255});
    }
    DrawRectangle(static_cast<int>(r.x + r.width * 0.5f - 8), static_cast<int>(r.y + r.height - 22),
                  16, 22, alive ? Color{143, 86, 44, 255} : Color{83, 83, 81, 255});
    DrawCircle(static_cast<int>(r.x + r.width * 0.5f + 5), static_cast<int>(r.y + r.height - 11), 2,
               alive ? Color{217, 167, 73, 255} : Color{60, 60, 60, 255});
    if (alive) {
        Rectangle sign{r.x + 6.0f, r.y - 31.0f, r.width - 12.0f, 15.0f};
        DrawRectangleRec(sign, Color{94, 64, 39, 255});
        DrawRectangleLinesEx(sign, 1.0f, Color{54, 37, 24, 255});
        DrawText(building.label.c_str(), static_cast<int>(sign.x + 4), static_cast<int>(sign.y + 3), 8, Color{244, 221, 161, 255});
    } else {
        DrawLineEx(Vector2{r.x + 10.0f, r.y - 25.0f}, Vector2{r.x + r.width - 10.0f, r.y - 10.0f}, 4.0f, Color{103, 38, 33, 255});
        DrawLineEx(Vector2{r.x + r.width - 10.0f, r.y - 25.0f}, Vector2{r.x + 10.0f, r.y - 10.0f}, 4.0f, Color{103, 38, 33, 255});
    }
}

inline void DrawRoadSign(Vector2 pos, const char* label) {
    DrawLineEx(Vector2{pos.x, pos.y}, Vector2{pos.x, pos.y + 26.0f}, 3.0f, Color{87, 60, 38, 255});
    Rectangle sign{pos.x - 36.0f, pos.y - 11.0f, 72.0f, 15.0f};
    DrawRectangleRec(sign, Color{112, 75, 43, 255});
    DrawRectangleLinesEx(sign, 1.0f, Color{62, 42, 26, 255});
    DrawText(label, static_cast<int>(sign.x + 3), static_cast<int>(sign.y + 3), 7, Color{245, 226, 176, 255});
}

inline void DrawBezierRoadVisual(Vector2 a, Vector2 b, Vector2 c, Vector2 d, float width, Color color) {
    Vector2 prev = a;
    for (int i = 1; i <= 36; ++i) {
        const Vector2 p = BezierPoint(a, b, c, d, static_cast<float>(i) / 36.0f);
        DrawLineEx(prev, p, width, color);
        DrawLineEx(prev, p, std::max(2.0f, width * 0.34f), Fade(Color{130, 100, 64, 255}, 0.28f));
        prev = p;
    }
}

inline void DrawPolylineRoadVisual(const std::vector<Vector2>& path, float width, Color color) {
    for (int i = 0; i + 1 < static_cast<int>(path.size()); ++i) {
        const Vector2 a = path[i];
        const Vector2 d = path[i + 1];
        const Vector2 dir{d.x - a.x, d.y - a.y};
        const Vector2 n = RoadNormal(a, d);
        const float curve = (i % 2 == 0 ? 34.0f : -28.0f);
        DrawBezierRoadVisual(a, Vector2{a.x + dir.x * 0.35f + n.x * curve, a.y + dir.y * 0.35f + n.y * curve},
                             Vector2{a.x + dir.x * 0.68f - n.x * curve * 0.55f, a.y + dir.y * 0.68f - n.y * curve * 0.55f},
                             d, width, color);
    }
}

inline void DrawMarketFlags() {
    const float phase = static_cast<float>(GetTime()) * 3.0f;
    const std::array<Vector2, 4> poles = {{{322, 270}, {558, 274}, {320, 432}, {560, 430}}};
    for (int i = 0; i < static_cast<int>(poles.size()); ++i) {
        const Vector2 p = poles[i];
        DrawLineEx(p, Vector2{p.x, p.y - 42.0f}, 3.0f, Color{82, 58, 35, 255});
        const float wave = std::sin(phase + static_cast<float>(i)) * 4.0f;
        const Color cloth = i % 2 == 0 ? Color{167, 55, 48, 255} : Color{49, 97, 145, 255};
        DrawTriangle(Vector2{p.x, p.y - 40.0f}, Vector2{p.x + 28.0f, p.y - 34.0f + wave},
                     Vector2{p.x, p.y - 26.0f}, cloth);
        DrawTriangle(Vector2{p.x, p.y - 26.0f}, Vector2{p.x + 24.0f, p.y - 22.0f - wave * 0.4f},
                     Vector2{p.x, p.y - 15.0f}, Fade(cloth, 0.84f));
    }
}

inline Color WorldBoxTileAccent(int col, int row, Color base) {
    const int delta = ((col * 17 + row * 31) & 3) - 1;
    auto channel = [delta](unsigned char c) {
        return static_cast<unsigned char>(std::clamp(static_cast<int>(c) + delta * 5, 0, 255));
    };
    return Color{channel(base.r), channel(base.g), channel(base.b), base.a};
}

inline void DrawWorldBoxResourceSprite(const TerrainTile& tile, int x, int y) {
    if (tile.reserve <= 0.0f) return;
    switch (tile.resource) {
        case TerrainResource::Wood:
            DrawRectangle(x + 11, y + 8, 10, 15, Color{94, 66, 39, 255});
            DrawRectangle(x + 6, y + 5, 20, 13, Color{38, 119, 52, 255});
            DrawRectangle(x + 9, y + 2, 14, 8, Color{56, 153, 66, 255});
            DrawRectangle(x + 7, y + 17, 18, 4, Color{31, 91, 43, 255});
            break;
        case TerrainResource::FertileSoil:
            for (int i = 0; i < 4; ++i) {
                DrawRectangle(x + 5 + i * 6, y + 6, 3, 20, Color{89, 135, 46, 255});
                DrawRectangle(x + 6 + i * 6, y + 6 + (i % 2) * 3, 4, 3, Color{226, 190, 72, 255});
            }
            break;
        case TerrainResource::Iron:
        case TerrainResource::Gold:
        case TerrainResource::Silver:
        case TerrainResource::Platinum:
        case TerrainResource::Coal:
        case TerrainResource::Salt: {
            const Color ore = TerrainResourceColor(tile.resource);
            DrawRectangle(x + 7, y + 15, 18, 9, Color{89, 91, 88, 255});
            DrawRectangle(x + 11, y + 10, 12, 10, ore);
            DrawRectangle(x + 16, y + 7, 9, 8, Color{158, 158, 151, 255});
            DrawRectangleLines(x + 7, y + 15, 18, 9, Fade(BLACK, 0.25f));
            break;
        }
        case TerrainResource::Water:
            DrawRectangle(x + 7, y + 11, 18, 11, Color{77, 157, 210, 255});
            DrawRectangle(x + 10, y + 9, 10, 3, Color{131, 197, 232, 255});
            break;
        case TerrainResource::None:
            break;
    }
}

inline void DrawWorldBoxBuildingTop(const TownBuilding& building) {
    Rectangle r = building.rect;
    r.x = std::floor(r.x / 8.0f) * 8.0f;
    r.y = std::floor(r.y / 8.0f) * 8.0f;
    r.width = std::max(24.0f, std::floor(r.width / 8.0f) * 8.0f);
    r.height = std::max(24.0f, std::floor(r.height / 8.0f) * 8.0f);

    Color body = building.wall;
    Color roof = building.roof;
    bool isHome = false;
    if (building.kind == BuildingKind::Farm) {
        body = Color{162, 128, 68, 255};
        roof = Color{212, 168, 58, 255};
    } else if (building.kind == BuildingKind::Mine) {
        body = Color{87, 89, 88, 255};
        roof = Color{54, 55, 55, 255};
    } else if (building.kind == BuildingKind::Lumberyard) {
        body = Color{145, 98, 52, 255};
        roof = Color{49, 130, 58, 255};
    } else if (building.kind == BuildingKind::Well) {
        body = Color{93, 137, 172, 255};
        roof = Color{123, 82, 48, 255};
    } else if (building.kind == BuildingKind::Plaza || building.kind == BuildingKind::MarketStall) {
        body = Color{205, 169, 89, 255};
        roof = Color{185, 58, 51, 255};
    } else if (building.kind == BuildingKind::Bank || building.kind == BuildingKind::Church) {
        body = Color{174, 166, 148, 255};
        roof = Color{86, 91, 112, 255};
    } else if (building.kind == BuildingKind::RichHome || building.kind == BuildingKind::MiddleHome ||
               building.kind == BuildingKind::CommonHome) {
        isHome = true;
        if (building.kind == BuildingKind::RichHome) {
            body = Color{239, 218, 178, 255};
            roof = Color{156, 62, 48, 255};
        } else if (building.kind == BuildingKind::MiddleHome) {
            body = Color{224, 204, 162, 255};
            roof = Color{178, 93, 51, 255};
        } else {
            body = Color{210, 192, 155, 255};
            roof = Color{147, 76, 42, 255};
        }
    }

    // Shadow underneath
    DrawRectangle(static_cast<int>(r.x + 4), static_cast<int>(r.y + 5), static_cast<int>(r.width), static_cast<int>(r.height), Fade(BLACK, 0.18f));
    // Main body
    DrawRectangleRec(r, body);
    // Roof
    DrawRectangle(static_cast<int>(r.x - 1), static_cast<int>(r.y), static_cast<int>(r.width + 2), 9, roof);
    // Roof highlight
    DrawRectangle(static_cast<int>(r.x + 2), static_cast<int>(r.y), static_cast<int>(r.width - 4), 3, Fade(WHITE, 0.14f));
    // Base shadow
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y + r.height - 6), static_cast<int>(r.width), 6, Fade(BLACK, 0.12f));
    // Border
    DrawRectangleLinesEx(r, 2.0f, Fade(BLACK, 0.28f));

    if (isHome) {
        // Two windows with frames
        DrawRectangle(static_cast<int>(r.x + 8), static_cast<int>(r.y + 14), 8, 8, Color{252, 226, 118, 255});
        DrawRectangleLines(static_cast<int>(r.x + 7), static_cast<int>(r.y + 13), 10, 10, Color{68, 48, 34, 255});
        DrawRectangle(static_cast<int>(r.x + r.width - 16), static_cast<int>(r.y + 14), 8, 8, Color{252, 226, 118, 255});
        DrawRectangleLines(static_cast<int>(r.x + r.width - 17), static_cast<int>(r.y + 13), 10, 10, Color{68, 48, 34, 255});
        // Door
        DrawRectangle(static_cast<int>(r.x + r.width / 2 - 5), static_cast<int>(r.y + r.height - 18), 10, 14, Color{82, 54, 39, 255});
        // Chimney
        DrawRectangle(static_cast<int>(r.x + r.width - 12), static_cast<int>(r.y - 10), 6, 14, Color{108, 74, 52, 255});
    } else if (building.kind == BuildingKind::Farm) {
        for (int yy = 0; yy < 4; ++yy) {
            DrawRectangle(static_cast<int>(r.x + 8), static_cast<int>(r.y + 12 + yy * 10), static_cast<int>(r.width - 16), 4,
                          yy % 2 == 0 ? Color{101, 151, 52, 255} : Color{225, 188, 72, 255});
        }
    } else if (building.kind == BuildingKind::Well) {
        DrawRectangle(static_cast<int>(r.x + r.width * 0.5f - 7), static_cast<int>(r.y + r.height * 0.5f - 7), 14, 14,
                      Color{62, 126, 185, 255});
    } else if (building.kind == BuildingKind::MarketStall) {
        // Stall awning stripe
        DrawRectangle(static_cast<int>(r.x + 4), static_cast<int>(r.y + 9), static_cast<int>(r.width - 8), 2, Fade(WHITE, 0.22f));
    }
}

inline void DrawWorldBoxTownBase(const TownMap& map, Rectangle visibleWorld) {
    DrawTerrainMap(GetTerrainMap(), visibleWorld);

    const int startCol = std::max(0, static_cast<int>(std::floor(visibleWorld.x / TownMap::CellSize)) - 1);
    const int startRow = std::max(0, static_cast<int>(std::floor(visibleWorld.y / TownMap::CellSize)) - 1);
    const int endCol = std::min(TownMap::Columns - 1,
                                static_cast<int>(std::ceil((visibleWorld.x + visibleWorld.width) / TownMap::CellSize)) + 1);
    const int endRow = std::min(TownMap::Rows - 1,
                                static_cast<int>(std::ceil((visibleWorld.y + visibleWorld.height) / TownMap::CellSize)) + 1);

    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            const int x = col * TownMap::CellSize;
            const int y = row * TownMap::CellSize;
            const TileType type = map.tiles[TileIndex(col, row)].type;
            if (type == TileType::Road || type == TileType::Plaza || type == TileType::MarketStall) {
                Color color = type == TileType::Road ? Color{196, 162, 98, 255} : Color{218, 188, 102, 255};
                DrawRectangle(x, y, TownMap::CellSize, TownMap::CellSize, WorldBoxTileAccent(col, row, color));
                // Cobblestone detail
                if ((col * 7 + row * 13) % 5 == 0) {
                    DrawRectangle(x + 8, y + 8, 6, 6, Fade(Color{226, 203, 128, 255}, 0.22f));
                }
                DrawRectangleLines(x, y, TownMap::CellSize, TownMap::CellSize, Fade(Color{116, 91, 54, 255}, 0.35f));
            } else {
                DrawRectangleLines(x, y, TownMap::CellSize, TownMap::CellSize, Fade(BLACK, 0.07f));
                // Random grass tufts on non-road tiles
                if ((col * 19 + row * 37) % 11 == 0) {
                    DrawRectangle(x + 4, y + 22, 4, 10, Color{78, 128, 60, 255});
                    DrawRectangle(x + 24, y + 20, 3, 12, Color{92, 142, 70, 255});
                    DrawRectangle(x + 16, y + 24, 5, 8, Color{68, 118, 52, 255});
                }
                // Small flowers
                if ((col * 23 + row * 41) % 13 == 0) {
                    DrawCircle(x + 12, y + 16, 2, Color{228, 108, 122, 255});
                    DrawCircle(x + 24, y + 22, 2, Color{242, 212, 84, 255});
                }
            }
        }
    }

    const TerrainMap& terrain = GetTerrainMap();
    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            const int tc = std::clamp((col * TownMap::CellSize + TownMap::CellSize / 2) / TerrainMap::TileSize, 0, TerrainMap::Columns - 1);
            const int tr = std::clamp((row * TownMap::CellSize + TownMap::CellSize / 2) / TerrainMap::TileSize, 0, TerrainMap::Rows - 1);
            const TerrainTile& tile = TerrainAt(terrain, tc, tr);
            if (tile.resource != TerrainResource::None && ((col * 13 + row * 7) % 3 == 0 || tile.resource == TerrainResource::FertileSoil)) {
                DrawWorldBoxResourceSprite(tile, col * TownMap::CellSize, row * TownMap::CellSize);
            }
        }
    }

    for (const auto& building : map.buildings) {
        DrawWorldBoxBuildingTop(building);
    }
}

inline bool UseWorldBoxVisualStyle() {
    return true;
}

inline void DrawTownMapBase(const TownMap& map, Rectangle visibleWorld) {
    if (UseWorldBoxVisualStyle()) {
        DrawWorldBoxTownBase(map, visibleWorld);
        return;
    }

    DrawTerrainMap(GetTerrainMap(), visibleWorld);

    const std::vector<Vector2> highStreet = {{44, 360}, {164, 314}, {306, 340}, {440, 352}, {576, 344}, {716, 382}, {844, 338}};
    const std::vector<Vector2> churchRoad = {{438, 352}, {466, 272}, {508, 198}, {552, 118}, {620, 68}};
    const std::vector<Vector2> manorRoad = {{70, 112}, {164, 92}, {254, 128}, {334, 226}, {406, 332}};
    const std::vector<Vector2> craftRoad = {{610, 84}, {698, 122}, {758, 226}, {706, 338}, {670, 478}, {764, 632}};
    const std::vector<Vector2> millRoad = {{112, 642}, {214, 558}, {330, 524}, {442, 442}, {552, 452}, {690, 532}, {812, 650}};
    const std::vector<Vector2> farmRoad = {{58, 284}, {126, 330}, {202, 392}, {246, 474}, {190, 594}};
    DrawPolylineRoadVisual(highStreet, 31.0f, Color{189, 151, 92, 255});
    DrawPolylineRoadVisual(churchRoad, 24.0f, Color{181, 145, 92, 255});
    DrawPolylineRoadVisual(manorRoad, 23.0f, Color{202, 174, 119, 255});
    DrawPolylineRoadVisual(craftRoad, 24.0f, Color{171, 139, 95, 255});
    DrawPolylineRoadVisual(millRoad, 26.0f, Color{188, 151, 99, 255});
    DrawPolylineRoadVisual(farmRoad, 22.0f, Color{193, 159, 99, 255});

    for (int i = 0; i < 90; ++i) {
        const float x = 18.0f + static_cast<float>((i * 73) % 830);
        const float y = 64.0f + static_cast<float>((i * 47) % 620);
        DrawCircleV(Vector2{x, y}, 1.2f + static_cast<float>(i % 3) * 0.5f,
                    i % 2 == 0 ? Fade(Color{88, 131, 65, 255}, 0.55f) : Fade(Color{124, 104, 73, 255}, 0.38f));
    }

    DrawCircleV(Vector2{440, 354}, 118.0f, Color{209, 185, 122, 255});
    DrawCircleV(Vector2{440, 354}, 96.0f, Color{218, 194, 131, 255});
    DrawCircleLines(static_cast<int>(440), static_cast<int>(354), 118.0f, Color{128, 92, 49, 255});
    DrawMarketFlags();

    for (const auto& building : map.buildings) {
        DrawTownBuilding(building);
    }

    DrawRoadSign(Vector2{362.0f, 304.0f}, "Market Street");
    DrawRoadSign(Vector2{514.0f, 248.0f}, "Church Row");
    DrawRoadSign(Vector2{160.0f, 116.0f}, "Manor Lane");
    DrawRoadSign(Vector2{704.0f, 154.0f}, "Craft Road");
    DrawRoadSign(Vector2{412.0f, 496.0f}, "Mill Lane");

    const std::array<Vector2, 17> trees = {{
        {38, 62}, {66, 108}, {812, 124}, {842, 178}, {804, 686}, {72, 704}, {132, 222}, {274, 280},
        {604, 110}, {846, 330}, {606, 610}, {118, 610}, {584, 542}, {326, 674}, {824, 526}, {28, 340}, {538, 84}
    }};
    for (Vector2 tree : trees) {
        DrawRectangle(static_cast<int>(tree.x - 4), static_cast<int>(tree.y + 7), 8, 17, Color{96, 68, 43, 255});
        DrawLine(static_cast<int>(tree.x), static_cast<int>(tree.y + 8), static_cast<int>(tree.x), static_cast<int>(tree.y + 22), Color{63, 43, 28, 255});
        DrawCircleV(Vector2{tree.x, tree.y - 8.0f}, 12.0f, Color{55, 112, 61, 255});
        DrawCircleV(Vector2{tree.x - 9.0f, tree.y + 2.0f}, 14.0f, Color{73, 139, 75, 255});
        DrawCircleV(Vector2{tree.x + 9.0f, tree.y + 3.0f}, 13.0f, Color{86, 154, 78, 255});
    }
    const std::array<Vector2, 20> flowers = {{
        {142, 525}, {155, 518}, {676, 96}, {690, 92}, {586, 462}, {604, 470}, {262, 226}, {276, 232}, {232, 690},
        {244, 704}, {602, 690}, {620, 684}, {36, 518}, {820, 250}, {842, 252}, {788, 386}, {806, 394}, {294, 74}, {310, 82}, {560, 226}
    }};
    for (int i = 0; i < static_cast<int>(flowers.size()); ++i) {
        DrawCircleV(flowers[i], 3.0f, i % 2 == 0 ? Color{216, 93, 111, 255} : Color{235, 208, 78, 255});
    }
}

} // namespace town
