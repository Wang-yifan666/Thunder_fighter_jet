# 路线图

## 已完成

### 阶段 0–4：逻辑与平台分离（v1.6）

原始项目是一个单体终端游戏（FTXUI）。这一阶段把它拆成了：

- `thunder_logic`：`GameWorld`（全部可变状态）、`GameSession`（非阻塞流程状态机）、
  `InputState` / `InputTracker`、`WorldSnapshot` / `SessionSnapshot`、
  `HighScore` + `ScoreStore` 接口
- `thunder_io`：`FileScoreStore`（唯一的文件 IO 实现）
- 终端前端改为单一非阻塞循环，SDL3 前端复用同一个会话
- GoogleTest 回归测试 + 逻辑层隔离检查（`tests/check_isolation.cmake`）

历史细节见 git log（每个批次一个提交）与 `docx/flowchart.md`。

### v1.7：产品化收尾

- 删除终端前端，只保留 SDL3 前端；可执行文件改名 `Naidan.exe`
- 玩家 / 敌机 / 血包 / 弹药包接入位图精灵，构建时内嵌进 exe
- 掉落物系统（血包 + 弹药包），掉落率随难度变化
- 拾取判定与伤害判定分离
- 敌机命中框 3×2、敌机不重叠
- 作弊模式改为默认关闭，主菜单 `F` 切换
- exe 图标 + 中英双语版本信息
- `.clang-format` 修复并全仓格式化
- 文档重写，截图改为工具生成（`tools/make_screenshots.py`）

## 待办

### 1. 持续集成（进行中）

`.github/workflows/build.yml` 已就位：MinGW + vcpkg（带二进制缓存）+ ctest +
无头冒烟 + 产物上传。**首次推送后需要确认能跑通**，尤其是：

- vcpkg 在 runner 上用 `x64-mingw-static` 从源码构建 SDL3_ttf 的耗时
- MSYS2 提供的 gcc 版本与本地 11.2.0 的行为差异

### 2. ~~数据路径改造~~（已完成）

`ResolveScorePath` 现在是：`NAIDAN_DATA_DIR` 环境变量 > exe 同目录可写 > 兼容旧布局
> `%APPDATA%\Naidan\`。写文件改用二进制模式，`highscore.txt` 也从版本控制移除
并加入 `.gitignore`，仓库不会再被存档弄脏。

配套测试 `tests/score_path_test.cpp` 覆盖了四种分支与写入格式。

### 3. ~~安装包（Inno Setup）~~（已完成）

`packaging/naidan.iss` + `tools/build_installer.ps1`：

- 每用户安装到 `%LOCALAPPDATA%\Programs\Naidan`，全程不需要管理员权限
- 开始菜单快捷方式、可选桌面图标、自带卸载程序
- 中英双语界面（简体中文语言包随仓库分发）
- 卸载时清理安装目录里的便携存档，不动 `%APPDATA%` 里的用户数据
- CI 打 tag 时自动构建安装包并作为 artifact 上传

实测：安装 → 启动 → 卸载 全流程通过，注册表项与安装目录都能干净清除。

### 4. 代码签名

用户下载 exe 时会遇到 SmartScreen / 浏览器拦截。根治途径：
[SignPath Foundation](https://signpath.org/) 对合格开源项目免费签名。

申请前置条件（已核对官方条款）：

- ✅ MIT 许可、无专有组件、活跃维护、功能有文档
- ✅ exe 已带完整版本元数据（SignPath 的 artifact 配置强制项）
- ⬜ 需要 CI 能产出可验证构建（见待办 1）
- ⬜ 需要 GitHub Releases 发布形式
- ⬜ 需要 `CODE_SIGNING.md`：指定话术、Authors/Reviewers/Approvers 三角色、隐私声明
- ⬜ 团队成员 GitHub 与 SignPath 均开启 MFA
- ⚠️ 项目声誉是自由裁量项，条款明确"不会给没人知道源码的项目签"

### 5. Android 验证

SDL3 官方支持 Android（NativeActivity + `SDL_main` + Gradle），
现有前端理论上可以直接移植，主要是：

- 触摸 → `RawInput` 映射
- 逻辑分辨率 832×496 在竖屏手机上会大量留黑边，需要自适应布局
- vcpkg 对 Android triplet 支持一般，大概率改用 SDL 官方 android 模板

计划先做 spike 验证可行性，再决定是否抽出 `thunder_view` 布局层。

## 发布流程

```powershell
# 1. 改版本号：CMakeLists.txt 的 project(VERSION) 与 inc/game/game_info.h
# 2. 提交并打 tag
git commit -am "release: v1.8"
git tag v1.8
git push && git push --tags
```

CI 会自动：构建 → 跑测试 → 冒烟 → 生成安装包 → 创建 GitHub Release，
挂上 `Naidan.exe`（便携版）与 `Naidan-1.8-setup.exe`（安装版）。

### 分发渠道

| 产物 | 位置 | 原因 |
|---|---|---|
| 安装包（4 MB） | 网站 `release/` + GitHub Releases | 国内直连比 GitHub 稳，体积也可控 |
| 便携版 exe（8 MB） | **仅** GitHub Releases | 体积大，不进任何仓库 |

网站上多放一份安装包意味着每次发版要手动做两件事：替换
`release/Naidan-<版本>-setup.exe`，以及改 `showcase.html` 里那一个链接的版本号。
如果哪天觉得麻烦，把这行链接改回 `releases/latest` 即可，其余不用动。

历史上塞进网站仓库的旧 exe 仍留在 git 历史里，但 tip 已经清理干净，
之后也不会再往仓库里加便携版。