# 运行流程图

描述当前（v1.7，单一 SDL3 前端）的实际执行路径。

## 主循环

```mermaid
flowchart TD
    Start(("main()")) --> Init["SDL_Init(VIDEO)<br>SdlFont::Init()<br>建窗口 832x496 + 渲染器"]
    Init --> Store["FileScoreStore<br>GameSession(store)"]
    Store --> Loop{"running ?"}

    Loop -- 是 --> Events["轮询 SDL 事件<br>QUIT / F11 / 键盘 → SdlInput"]
    Events --> Acc["accumulator += 实际耗时"]
    Acc --> Steps{"accumulator >= 16ms<br>且 steps < 5 ?"}
    Steps -- 是 --> Step["session.Step(input, 16ms)<br>（贪睡时最多补 5 步）"]
    Step --> Steps
    Steps -- 否 --> Exit{"exit_requested ?"}
    Exit -- 是 --> End
    Exit -- 否 --> Snap["snapshot = session.Snapshot()"]
    Snap --> Draw["SdlRender::Draw(snapshot, fps, seconds)"]
    Draw --> Delay["--no-vsync 时软限帧"]
    Delay --> Loop
    Loop -- 否 --> End(("退出"))
```

## GameSession 状态机

```mermaid
flowchart TD
    Menu["Menu 主菜单"] -- "1" --> Diff["DifficultySelect"]
    Menu -- "2" --> Board["Leaderboard"]
    Menu -- "Q" --> Exit(("退出程序"))
    Menu -. "F（任意时刻）" .-> Cheat["切换作弊模式<br>提示 1.5 秒后自动隐藏"]

    Diff -- "1 / 2 / 3" --> Play["Playing"]
    Diff -- "B" --> Menu
    Board -- "B" --> Menu
    Board -- "Esc" --> Exit

    Play -- "Q" --> Menu
    Play -- "Esc" --> Exit
    Play -- "P（按下沿）" --> Pause["暂停（不调用 Tick）"]
    Pause -- "P" --> Play
    Play -- "life <= 0" --> Over["GameOver<br>结算 + 写入排行榜"]
    Over -- "R" --> Menu
    Over -- "Q" --> Exit
```

## 一帧的 Tick 顺序（GameWorld::Tick）

```mermaid
flowchart TD
    A["enemy_move_counter++"] --> B["elapsed += dt<br>elapsed_seconds 取整"]
    B --> C["无敌倒计时 -= dt"]
    C --> D{"到移动间隔？"}
    D -- 是 --> D1["MoveEnemies<br>（禁止走进已被占用的格子）"]
    D -- 否 --> E
    D1 --> E{"pending 非空？"}
    E -- 是 --> E1["SpawnEnemiesFromPending<br>（顶部 kEnemyHeight 行全空才刷）"]
    E -- 否 --> F
    E1 --> F["UpdateLevel：每 10 秒升级<br>按等级收紧移动间隔"]
    F --> G["AccrueScore：每跨 1 秒 +10"]
    G --> H["开火输入 → 生成子弹（弹药 > 0）"]
    H --> I["按输入移动玩家"]
    I --> J["作弊键（会话层已门控，未开启时全为 false）"]
    J --> K["玩家位置钳制到棋盘内"]
    K --> L["UpdatePickups + CheckPickupCollect<br>（拾取用视觉身体 4 行判定）"]
    L --> M["CheckPlayerCollision<br>（伤害用 3x1 命中框）"]
    M --> N["UpdateBullets<br>命中 → 加分 +5 弹药 + 概率掉落"]
```

## 分层

```mermaid
flowchart LR
    subgraph platform["平台相关"]
        SDL["src/sdl<br>渲染 / 输入 / 入口"]
    end
    subgraph io["thunder_io"]
        FS["FileScoreStore<br>ScoreStore 接口实现"]
    end
    subgraph logic["thunder_logic（零平台依赖）"]
        Session["GameSession<br>流程状态机"]
        World["GameWorld<br>对局状态"]
        Snap["WorldSnapshot /<br>SessionSnapshot"]
        HS["HighScore"]
    end

    SDL -- "RawInput + dt" --> Session
    Session --> World
    World --> Snap
    Snap -- "只读快照" --> SDL
    Session --> HS
    HS -. "ScoreStore 接口注入" .- FS
```

逻辑层不包含任何 SDL / windows.h / 文件 IO / 系统时钟调用，
由 `tests/check_isolation.cmake` 在 ctest 中强制校验。
