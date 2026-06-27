# Town Market Simulator

A standalone C++17/raylib 2D town economy simulator prototype.

## Features

- 32 resident agents with professions, needs, inventories, money, and simple relationships.
- Dynamic supply/demand market pricing.
- Goods across raw materials, processed goods, tools, vehicles, and luxury items.
- Manual external event triggers.
- 2D town map, resident movement, price charts, market panel, and resident detail panel.
- CSV export of price history.

## Controls

- `Space`: Pause/resume
- `N`: Step one day while paused
- `1`: Bumper Harvest
- `2`: Crop Failure
- `3`: War Demand
- `4`: Trade Route Broken
- `5`: Plague Scare
- `6`: New Ore Vein
- `E`: Export CSV to `price_history.csv`
- `Left click`: Select a resident

## Build

Install CMake and a C++17 compiler first. On Windows, Visual Studio Build Tools 2022 or MinGW both work.

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\TownMarket.exe
```

For single-config generators such as Ninja or MinGW Makefiles:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\TownMarket.exe
```

The CMake project downloads raylib automatically through `FetchContent`.

You can also use the helper script:

```powershell
.\scripts\build.ps1 -Config Release
```
