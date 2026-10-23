# 雷霆战机 v1.6

## 项目简介

**雷霆战机（Thunder Fighter）** 是一款纵向射击游戏：玩家操控飞机在有限生命与弹药条件下躲避敌人、发射子弹击败敌机，尽可能获得更高分数。

目前有两个前端：

- **终端版**（FTXUI）：原有版本，玩法与界面保持不变；
- **SDL3 图形版**：最小可玩版本，使用同一套游戏逻辑。

---

## 编译与运行

### 编译环境

- C++17
- Windows x64
- MinGW-w64 + CMake
- vcpkg（`ftxui`、`gtest`）
- SDL3（可选，仅图形版需要）

### 构建终端版

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
      -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake `
      -DVCPKG_TARGET_TRIPLET=x64-mingw-static
cmake --build build
```

产物：

```
build/Thunder_fighter_jet.exe          # 动态链接
build/Thunder_fighter_jet_static.exe   # 静态链接，可直接运行
```

### 运行测试

```powershell
ctest --test-dir build --output-on-failure
```

测试覆盖游戏世界（重置 / 边界 / 子弹 / 无敌 / 升级 / 计分）、会话流程、排行榜规则与终端字符网格。

### 构建 SDL3 图形版（可选）

先安装 SDL3（与 ftxui / gtest 使用同一个 triplet）：

```powershell
vcpkg install sdl3:x64-mingw-static
```

之后的构建命令与终端版完全相同，配置阶段会自动启用 SDL 目标：

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
      -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake `
      -DVCPKG_TARGET_TRIPLET=x64-mingw-static
cmake --build build
```

产物：`build/Thunder_fighter_jet_sdl.exe`。

- 未找到 SDL3 时，配置阶段会提示并自动跳过该目标，终端版不受影响；
- 若 SDL3 不在 vcpkg 中，可用 `-DCMAKE_PREFIX_PATH=<SDL3 安装目录>` 指定；
- 窗口可缩放（保持 800×432 逻辑比例并留黑边），`F11` 切换全屏。

---

## 游戏操作

| 按键    | 功能                            |
| ------- | ------------------------------- |
| `W`     | 向上移动                        |
| `A`     | 向左移动                        |
| `S`     | 向下移动                        |
| `D`     | 向右移动                        |
| `Space` | 发射子弹                        |
| `R`     | 结算界面重新开始                |
| `Q`     | 游戏中返回主菜单 / 结算界面退出 |
| `P`     | 暂停游戏（按下触发，长按不重复） |
| `Esc`   | 直接退出程序                    |

SDL3 版键位相同，HUD 使用英文 ASCII 标签。

---

## 游戏界面展示

### 排行榜界面
<p align="center">
   <img src="assets/TOP.png" alt="排行榜界面" width="600" />
</p>

### 游戏结束 / 结算界面
<p align="center">
   <img src="assets/END.png" alt="游戏结束界面" width="600" />
</p>

---

## 已实现功能

### 核心玩法
- 玩家飞机移动（WASD）
- 敌人随机生成与下落
- 子弹系统（多发子弹 + 有限弹药）
- 子弹命中敌人并消灭
- 击败敌人加分
- 剩余弹药结算加分

### 游戏系统
- 生命值系统（扣血 + 无敌时间）
- 帧率实时显示
- 游戏运行时间统计
- 等级系统（随时间提升难度）
- 游戏结束结算界面（可重复游玩）

### 排行榜系统
- 本地保存历史最高分 TOP 5
- 自动排序并持久化
- 显示本局得分在排行榜中的排名
- 提示是否创造最高分 / 进入前五

### 代码结构
- 游戏逻辑（`thunder_logic`）与平台无关：不依赖 FTXUI / SDL3 / windows.h，不读系统时钟
- 终端前端（`thunder_core`）与 SDL3 前端（`Thunder_fighter_jet_sdl`）共享同一套逻辑
- 固定 16ms 逻辑步长，SDL 版渲染帧率与显示器刷新率解耦
- SDL 版使用色块精灵、静态星空与可缩放窗口（内置 5×7 点阵字体，无字体库依赖）

---

## 程序结构概览

```text
Thunder_fighter_jet/
├── inc/
│   ├── game/                  # 平台无关的游戏逻辑
│   │   ├── game_config.h      # 数值常量
│   │   ├── game_types.h       # 难度
│   │   ├── enemy.h            # 敌机数据
│   │   ├── game_world.h       # 对局状态与更新
│   │   ├── game_session.h     # 非阻塞流程状态机
│   │   ├── input_state.h      # 输入快照 / 边沿检测
│   │   ├── world_snapshot.h   # 世界只读快照
│   │   ├── session_snapshot.h # 会话只读快照
│   │   ├── highscore.h        # 排行榜规则
│   │   └── score_store.h      # 分数存储接口 + 内存实现
│   ├── io/file_score_store.h  # 文件存储实现
│   ├── def.h                  # 终端前端
│   ├── cell_grid.h            # 快照 → 字符网格
│   ├── render_console.h
│   ├── input_console.h
│   └── console_utils.h
├── src/
│   ├── game/                  # thunder_logic 实现
│   ├── io/                    # thunder_io 实现
│   ├── sdl/                   # SDL3 前端
│   ├── def.cpp / main.cpp     # 终端前端
│   └── cell_grid.cpp 等
├── tests/                     # GoogleTest 回归测试
├── docx/                      # 流程图 / 迁移计划 / 界面基线
├── build/                     # 构建目录（CMake 生成）
└── highscore.txt              # 本地排行榜数据
```

运行流程（两个前端一致）：

```
程序启动
   ↓
GameSession 状态机：菜单 → 难度选择 → 游戏中 → 结算 → 菜单
   ↓
GameWorld::Tick(input, dt) 推进对局（暂停时不调用）
   ↓
Snapshot（WorldSnapshot / SessionSnapshot）→ 终端网格 / SDL 绘制
```

### 调试/作弊指令 (Cheats)

游戏中按数字键（按住即重复触发）：

- `1`: 上帝模式
- `2`: 自杀
- `3`: 加子弹 (+100)
- `4`: 加分 (+1000)
- `5`: 无敌模式开关
- `6`: 增加生命
- `7`: 秒杀全屏敌人
- `8`: 清空当前敌人
- `9`: 强制升级关卡

---

## 版本更新记录

- v1.6
  - 逻辑与平台分离：新增 `thunder_logic` / `thunder_io`，终端与 SDL3 共享同一套游戏逻辑
  - 新增 SDL3 图形版（最小可玩：菜单 / 难度 / 游戏 / 暂停 / 结算）
  - 新增 GoogleTest 回归测试与隔离检查
  - 修复跨局状态污染（升级计时器、刷怪优先级常量）
  - 暂停键改为按下触发；`Esc` 在游戏中直接退出
- v1.5
  - 添加难度选择
  - 主菜单加入排行榜
- v1.4
  - 新增主菜单
  - 完成上帝模式
  - 引入状态机与基础单元测试
- v1.3
  - 完成游戏结束后的重启
  - 函数重构

## License

This project is licensed under the MIT License.
