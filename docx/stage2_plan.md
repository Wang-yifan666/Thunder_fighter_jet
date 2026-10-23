# Stage 2 执行计划（修订版）

本文件是 `MIGRATION_PLAN.md` 中 Stage 2 的可执行细化版本，记录批次、依赖、验收标准与已确认的决策。
`MIGRATION_PLAN.md` 仍作为长期路线图，本文件只描述当前阶段。

## 已确认的决策

| 项 | 决策 |
|---|---|
| SDL3 | 允许安装；B8 前优先评估 vcpkg Manifest Mode，若迁移影响现有环境则该批次沿用现有安装方式 |
| SDL HUD | 先使用英文 ASCII 标签，不引入字体渲染依赖 |
| Esc | 直接退出程序（与 README 一致），作为独立行为变更提交 |
| 测试框架 | GoogleTest（vcpkg `x64-mingw-static`），沿用现有 MinGW + CMake 工具链 |
| 平台 | 终端版（FTXUI）与 SDL 版并存，终端版在 B0–B6 期间行为不变 |

## 目标分层

```
thunder_logic   纯 C++17：GameWorld / InputState / GameSession / ScoreStore(接口)
                WorldSnapshot / SessionSnapshot / HighScore(排序规则)
                禁止：windows.h、FTXUI、SDL3、std::cout、system()、sleep、系统时钟
thunder_io      仅 std：FileScoreStore 等文件实现（逻辑层不得依赖）
thunder_core    终端前端：input_console / console_utils / cell_grid / render_console
thunder_sdl     SDL3 前端（B8 新增）
tests           仅链接 thunder_logic（终端渲染相关测试单独一个测试目标）
```

## 关键约束

- **暂停不推进模拟**：`GameSession` 在暂停分支直接返回，不调用 `GameWorld::Tick`。
- **InputTracker 唯一所有**：每个 `GameSession` 各自持有唯一实例；前端只负责产生 `RawInput`。
- **RawInput 需要按下事件**：`RawInput` 同时携带 `down` 状态与本帧 `pressed` 事件，
  避免 SDL 事件式输入在"快速按下→释放"时丢失输入；轮询式后端（终端）`pressed` 为空，
  由 `InputTracker` 根据 `down` 变化补出边沿。
- **固定逻辑频率**：SDL 前端用累加器以固定步长（16ms）推进 `GameWorld`，
  渲染帧率与显示器刷新率解耦；不追加大于上限的补帧。
- **跨局状态**：不得存在函数级 `static` 或命名空间级可变全局量保存对局状态。

## 批次与依赖

```
B0 基线 ─► B1 跨局状态修复 ─► B2 纯搬迁 ─► B3 回归测试
                                              │
                              ┌───────────────┼────────────────┐
                              ▼               ▼                ▼
                         B4 RNG 迁移        B5 ScoreStore 接口 + GameSession
                              │               │
                              │               ├─► B6 暂停键修复（行为提交）
                              │               └─► B7 Esc 修复（行为提交）
                              │                        │
                              └────────────────────────┴─► B8 SDL3 最小可玩（固定步长）
                                                               │
                                                               ▼
                                                    B9 终端采用 GameSession
                                                               │
                                                               ▼
                                                    B10 排行榜注入收尾 + 清理文档
```

| 批次 | 内容 | 提交性质 |
|---|---|---|
| B0 | 分支、干净构建、GTest 骨架、界面基线 | chore/test |
| B1 | `static last_level_up_time`、enemy 批次常量、`ResetGame` 漏重置字段 | fix（行为修复） |
| B2 | 抽出 `thunder_logic`/`GameWorld`/`InputState`/`WorldSnapshot`/`SessionSnapshot`；`cell_grid` 进终端层 | refactor（纯搬迁） |
| B3 | 回归测试：重置/边界/子弹/无敌/升级/计分/隔离检查/字符网格 golden | test |
| B4 | `rand()` → 世界自有 `std::mt19937`（可注入 seed） | 行为变更 |
| B5 | `ScoreStore` 接口（逻辑层）+ `FileScoreStore`（IO 层）+ 非阻塞 `GameSession` | feat |
| B6 | 暂停键改为按下沿切换 | 行为变更 |
| B7 | Esc 直接退出 | 行为变更 |
| B8 | SDL3 最小可玩可执行文件（固定步长、英文 HUD） | feat |
| B9 | 终端改为单一非阻塞循环，采用 `GameSession` | refactor（行为不变） |
| B10 | 排行榜 IO 注入收尾、死代码清理、文档与版本号 | refactor |

## 每批验收标准

1. `cmake --build build -j` 全目标成功（含当时已存在的前端目标）。
2. `ctest --test-dir build --output-on-failure` 全绿。
3. 终端界面与 `docx/baseline/` 比对无意外差异。
4. 提交说明包含：改动文件清单、行为是否变化、验证命令与结果。
5. 纯搬迁批次（B2、B9）不得夹带行为变更；行为变更（B1、B4、B6、B7）各自独立提交。

## 测试清单（B3 起）

- 重置：活动敌机清空 vs pending 数量符合难度公式；难度初始生命/弹药。
- 玩家边界：左/上到 `(0,0)`，右/下到 `(95,39)`。
- 子弹：命中消耗并加分/补弹；飞出顶部移除且不加分。
- 无敌：同一接触只扣 1 点，1 秒内不再扣；作弊无敌不掉血。
- 升级：1 秒步进 10 秒升 1 级；单次跨 30 秒只升 1 级（现状语义锁定）。
- 计分：每跨 1 秒 +10；单次跨 5 秒只 +10（现状语义锁定）。
- 隔离：逻辑层源码不得出现平台头/终端输出/系统时钟。
- 字符网格：人工构造 `WorldSnapshot` 的 golden 比对。
- Session：菜单键位、难度选择、Q 回菜单、R 重开、结算排名、暂停期间时间不推进。

## 完成情况

| 批次 | 提交 | 结果 |
|---|---|---|
| B0 基线 | `1a4dd5e`, `089e124` | 分支、干净构建、GTest 骨架、菜单基线 |
| B1 跨局状态 | `91e2b9f` | `static` 计时器与刷怪全局量修复（独立提交） |
| B2 纯搬迁 | `8d91ed2` | `thunder_logic` / `thunder_io` / `GameWorld` / `InputState` |
| B3 测试 | `2374109` | 26 个用例 + 隔离检查 + 字符网格 golden |
| B4 RNG | `b68958b` | RNG 归 `GameWorld` 所有，seed 可注入（独立提交） |
| B5 会话 | `2024968` | `ScoreStore` 接口、`FileScoreStore`、非阻塞 `GameSession` |
| B6 暂停 | `f33aa3f` | 暂停改为按下沿触发（独立提交） |
| B7 Esc | `9d1aef7` | 游戏中 Esc 直接退出（独立提交） |
| B8 SDL3 | `91e537d` | 最小可玩前端、固定步长、无头冒烟 |
| B9 终端会话化 | `410694c` | 单循环前端，界面文案不变 |
| B10 清理 | `eaa4b0f` + 文档提交 | 死代码、警告、文档与版本号 |

### 与计划的偏差

1. **vcpkg Manifest Mode 仍未采用**：vcpkg 自举需要从 GitHub Releases 下载
   ninja 1.13.1 与 7-Zip 25.01，开发机上 WinHTTP 下载时好时坏；经典模式可用，
   Manifest Mode 试验在依赖引导阶段失败，待下载通道稳定后再评估。
2. **SDL3 已回到 vcpkg**：`vcpkg install sdl3:x64-mingw-static`（3.2.24）安装成功，
   CMake 直接 `find_package(SDL3 CONFIG)`，不再需要 `CMAKE_PREFIX_PATH`；
   过渡期使用的 `%TEMP%/tf-sdl3-prefix` 本地构建不再必需。
3. **版本号**：游戏内 HUD 版本串由 `v1.4` 提升到 `v1.6`，与 README 对齐（有意变更，
   菜单基线不受影响）；已打 annotated tag `v1.6`。
4. **SDL 润色**（提交 `6acf39c`）：窗口可缩放、F11 全屏、组合色块精灵与静态星空；
   逻辑与终端版不受影响。
5. **GUI 子系统**：SDL 版以 `WIN32_EXECUTABLE` 构建，双击不再附带控制台窗口；
   调试可用 `-DTHUNDER_SDL_CONSOLE=ON` 改回控制台子系统；
   无头冒烟需用 `Start-Process -Wait -RedirectStandardOutput` 等待并取回输出。
6. **视觉动画**：星空按深度缓慢滚动、无敌期间玩家闪烁（纯绘制层，不影响判定）；
   新增 `--help`。

