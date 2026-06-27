---
name: townmarket
description: TownMarket 中世纪小镇经济模拟项目 — 你的主项目，每次会话必读
---

# TownMarket 项目永久记忆

## 你是谁
Alice，Win 研发主管。你负责 TownMarket 项目的所有 C++ 编码。

## 项目位置
- 仓库：`C:\Users\zhang\Projects\TownMarket\`
- 主文件：`src\main.cpp`（229KB）
- 编译：`cmake --build build --config Release -- /m:1`
- 运行：`build\Release\TownMarket.exe --chart --ticks=1000`
- 依赖：C++17 + raylib 2D

## 项目状态（截至 2026-06-22，Iter 12 进行中）

### 当前经济数据
| 指标 | 值 |
|------|-----|
| 人口 | 320 |
| 死亡 | 0（生存底线生效 ✅）|
| 失业率 | 86.6% |
| 通膨 | -20.2% |
| GDP 增速 | -0.02% |

### Iter 12 正在推进
完整计划见 `C:\Users\zhang\HermesControl\alice_iter12_plan.txt`

五个阶段：
- **阶段 A**：商品系统重构（质量分层 + 原材料→成品链路 + 品牌差异）
- **阶段 B**：信息不对称 + 消费心理（居民非完全理性、价格发现）
- **阶段 C**：小镇政府（税收、福利、公共工程就业）
- **阶段 D**：央行升级（GDP统计、利率调节、货币发行/回收）
- **阶段 E**：可视化升级（raylib图表、JSON输出）

执行顺序：A→B→C+D（并行）→E

## 教训（每次写代码前读一遍）
1. **不要新建 HTML/网页项目** — 这是 C++ + raylib 项目
2. **单步执行** — 不确定时先读文件再改
3. **编译前先 taskkill 旧进程**
4. **不要编造验证结果** — Adam 会亲自跑 --chart 验证
5. **所有输出文件放** `C:\Users\zhang\HermesControl\`，命名 `alice_town_iter12_X.txt`
6. **改完必须编译验证**，编译失败不算交付
7. **main.cpp 已 229KB，必要时拆文件**
