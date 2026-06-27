## 2026-06-22 Iter 12 Phase A

- Refactored goods into RawMaterial / Intermediate / FinishedGood without changing the aggregate inventory model.
- Added 1-5 star quality sampling with normal distribution; quality now adjusts nominal base value and production cost.
- Added company provenance for factory output through `producerCompanyId` and production ledger `companyId`.
- Established clearer chains: ore -> iron -> tools, grain -> flour -> bread, cotton/wool -> cloth -> clothes.
- Verified Release build with MSBuild because local `cmake --build ... -- /m:1` hung in this shell; `--chart --ticks=500` completed without crash.

## Iter 12 Phase A/B - 2026-06-22
- Completed A3 company marking: `Good::producingCompanyId` and `TradeRecord::producingCompanyId` are populated for factory-backed goods/trades.
- Completed core A2 material chains: `ProductionOrder` alias added and factory recipes now enforce ore->iron, grain/water->bread, wood->furniture through factory inventory checks.
- Completed B1/B2 information asymmetry: resident `priceKnowledge`, `qualityPreference`, class-based search radius, perceived prices, quality bid premium, and company premium now affect purchases.
- Removed random `marketNoise` from daily repricing; event multipliers are damped halfway toward neutral.
- Validation: direct x64 cl/link build succeeded after CMake/MSBuild wrapper hung in `cl.exe`; 1000 tick chart run completed with deaths=0. Output: `C:\Users\zhang\HermesControl\alice_iter12_AB.txt`.
- Residual risk: economy still shows strong deflation and high factory bankrupt/restart churn; leave for Phase C/D monetary/fiscal balancing.
