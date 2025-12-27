#  游戏运行流程图 (Game Execution Flow)

本文档描述了 `Thunder Fighter Jet` 的核心执行流程。

##  整体架构流程 (Mermaid Diagram)

```mermaid
flowchart TD
    %% 节点样式
    classDef startend fill:#f9f,stroke:#333,stroke-width:2px;
    classDef process fill:#e1f5fe,stroke:#0277bd,stroke-width:2px;
    classDef decision fill:#fff9c4,stroke:#fbc02d,stroke-width:2px;
    classDef subfunction fill:#e0f2f1,stroke:#00695c,stroke-width:1px,stroke-dasharray: 5 5;

    %% 程序启动
    Start(("程序启动<br>main")):::startend --> Init["构造 ThunderFighter<br>Constructor"]:::process
    Init --> LoadHS["加载最高分<br>highscore_.Load()"]:::subfunction
    Init --> InitEnemies["初始化敌人池<br>Make_enermy()"]:::subfunction
    Init --> MainLoop{"主循环<br>IsRunning() ?"}:::decision

    %% 主循环
    MainLoop -- True --> Run["调用 Run()"]:::process

    %% Run 内部：状态分发
    Run --> StateCheck{"当前 GameState ?"}:::decision

    %% ===== 主菜单 =====
    StateCheck -- Menu --> ShowMenu["显示主菜单<br>ShowMenu()"]:::process

    %% 主菜单：开始游戏 -> 难度选择
    ShowMenu -- "按 1" --> DiffMenu["选择难度<br>ShowDifficultyMenu()"]:::process
    DiffMenu -- "选定难度" --> ResetGame["ResetGame()"]:::process
    ResetGame --> SetPlaying["state = Playing"]:::process
    SetPlaying --> ReturnRun1["返回 Run()"]:::process

    %% 主菜单：排行榜（可选）
    ShowMenu -- "按 2 (排行榜)" --> Leaderboard["排行榜页面<br>ShowLeaderboard()"]:::process
    Leaderboard -- "按 B " --> ShowMenu

    %% 主菜单：退出
    ShowMenu -- "按 3 (退出)" --> ExitSet1["state = Exit<br>running_=false"]:::process
    ExitSet1 --> MainLoop

    %% ===== 游戏进行中 =====
    StateCheck -- Playing --> GameLoop{"Playing && !ShouldExit()"}:::decision

    GameLoop -- True --> DrawFrame["DrawFrame()"]:::process

    %% DrawFrame 内部
    subgraph DrawFrame_Logic [DrawFrame 内部逻辑]
        direction TB

        CheckBack{"按下 Q ?"}:::decision
        CheckBack -- Yes --> BackMenu["Back_to_menu()<br>state=Menu"]:::process
        CheckBack -- No --> PauseCheck{"按下 P ?"}:::decision

        PauseCheck -- Yes --> TogglePause["切换暂停<br>修正时间"]:::process
        PauseCheck -- No --> IsPaused{"is_paused_ ?"}:::decision

        IsPaused -- Yes --> RenderPause["绘制 PAUSED"]:::process
        IsPaused -- No --> LogicUpdate["逻辑更新"]:::process

        LogicUpdate --> CoreFuncs["核心逻辑调用"]:::subfunction

        subgraph LogicFunctions [逻辑调用顺序]
            direction TB
            F1["MoveEnemies (按 interval)"]
            F2["SpawnEnemiesFromPending (pending 非空)"]
            F3["Level() / score()"]
            F4["输入处理<br>WASD / Space / Cheats"]
            F5["CheckPlayerCollision()"]
            F6["UpdateBullets()"]

            F1 --> F2 --> F3 --> F4 --> F5 --> F6
        end

        CoreFuncs --> RenderGame["渲染游戏画面"]:::process
    end

    DrawFrame --> Sleep["sleep_for 16ms"]:::process
    Sleep --> GameLoop

    %% ===== 游戏结束（离开 Playing 循环） =====
    GameLoop -- False --> LifeCheck{"life_number <= 0 ?"}:::decision

    %% 不是死亡：可能是 ESC 或 state 切换到 Menu
    LifeCheck -- No --> ReturnRun2["返回 Run()"]:::process
    ReturnRun2 --> MainLoop

    %% 死亡：结算
    LifeCheck -- Yes --> GameOver["结算流程"]:::process
    GameOver --> CalcScore["子弹结算 + 排名计算"]:::subfunction
    CalcScore --> SubmitHS["highscore_.Submit(score_)"]:::subfunction

    %% 结算界面
    SubmitHS --> OverScreen{"结算界面"}:::decision

    OverScreen -- "按 R" --> MenuBack["state = Menu"]:::process
    MenuBack --> MainLoop

    OverScreen -- "按 Q" --> ExitSet2["state = Exit<br>running_=false"]:::process
    ExitSet2 --> MainLoop

    %% 程序结束
    MainLoop -- False --> End(("程序结束")):::startend
