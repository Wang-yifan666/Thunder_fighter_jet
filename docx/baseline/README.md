# 界面基线（B0）

本目录保存 B0 阶段的界面基线，用于在后续重构批次中判断"终端界面是否被改动"。

## 采集环境

| 项 | 值 |
|---|---|
| 分支 | `refactor/logic-separation`（基于 `main` 的 `3261f72`） |
| 编译器 | MinGW-w64 g++ 11.2.0（`D:\program\mingw`） |
| 构建 | CMake 4.4.3，`MinGW Makefiles`，`CMAKE_BUILD_TYPE=Release` |
| 依赖 | vcpkg `x64-mingw-static`（ftxui、gtest） |
| 可执行文件 | `build/Thunder_fighter_jet_static.exe` |

## 已自动采集

### `menu_screen.txt`

主菜单单帧文本，直接取自真实可执行文件的 stdout（ANSI 转义已剥离，换行统一为 LF）。

```
SHA256  1807F0D12E580ABC723FEB26E93EF6970AA5A947B7BEFFACBF2374AC7AD70712
字节数  259
```

重新采集方法（在仓库根目录执行）：

```powershell
$out = "$env:TEMP\menu_raw.txt"
$p = Start-Process -FilePath '.\build\Thunder_fighter_jet_static.exe' `
     -WorkingDirectory '.\build' -RedirectStandardOutput $out -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 4          # 需等待 stdout 缓冲区刷出若干帧
Stop-Process -Id $p.Id -Force
# 提取单帧 → docx/baseline/menu_screen.txt，见 git 历史中的 B0 提交说明
```

说明：

- 菜单使用 `system("cls")` 清屏，重定向到文件时该语句无输出，因此帧之间没有分隔符；
  提取时按 `================================\n        雷 霆 战 机` 起始、`  按 1 / 2 / 3 进行选择...` 后结尾截取。
- 采集时工作目录为 `build/`，因此 `highscore.txt` 解析为仓库根目录的文件（只读，不写入）。

## 尚未自动采集（需要真实键盘输入）

以下 5 个界面必须在交互式终端中人工截图/抄录后补入本目录。可通过游戏内的按键到达：

| 界面 | 到达方式 | 需要确认的元素 |
|---|---|---|
| 难度选择 | 主菜单按 `1` | 标题"选 择 难 度"；`[1] 简单 (Easy)`、`[2] 普通 (Normal)`、`[3] 困难 (Hard)`；`[B] 返回主菜单` |
| 排行榜 | 主菜单按 `2` | 标题"排 行 榜 TOP 5"；5 行 `序号. 分数 分`；`[B]返回主菜单` |
| 游戏进行中 | 选难度后进入 | 标题 `雷霆战机 v1.6`；`帧率 / 时长 / level / 剩余敌人 / 分数 / 最高分` 一行；`按Q退出, P暂停, 按WASD移动, 空格开火  |  子弹: N`；98×40 场景；底部"剩余生命"行 |
| 暂停 | 游戏中按 `P` | 场景第 20 行居中显示 `=== P A U S E D ===` |
| 游戏结束结算 | 生命耗尽 | `=== 游戏结束 ===`、`本局得分: N 分`、`历史最高分 TOP 5` 列表、排名提示语句、`[R] 返回主菜单`、`[Q] 退出游戏` |

> B3 阶段的 `Golden.CellGrid` 测试会用一个**人工构造的 `WorldSnapshot`**（不依赖随机种子注入）
> 锁定游戏画面的字符网格，覆盖上表"游戏进行中 / 暂停"两项的像素级回归；
> 菜单类界面仍以上表的文本/截图比对为准。

## 已知基线与现状的差异（有意变更，均已在对应提交中说明）

| 批次 | 差异 | 说明 |
|---|---|---|
| B6 | 暂停改为按下沿触发 | 长按 `P` 只切换一次；界面文字不变 |
| B7 | 游戏中 `Esc` 直接退出 | 与 README 一致；界面文字不变 |
| B10 | 游戏内标题版本串 `v1.4` → `v1.6` | 版本号对齐；菜单基线不受影响 |

## 使用规则

1. 任何批次若改变了上述界面的可见内容，必须在提交说明中列出差异与原因。
2. 若差异属于有意修复（例如 B7 暂停键行为、B8 Esc 修复），需同时更新本目录的基线，
   并在提交说明中标注"基线更新"。
3. 未经基线更新的界面差异一律视为回归。
