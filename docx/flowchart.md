# 🛠️ 游戏运行流程图 (Game Execution Flow)

本文档描述了 `Thunder Fighter Jet` 的核心执行流程。

## 📊 整体架构流程 (Mermaid Diagram)

```mermaid
flowchart TD
    %% 节点样式定义
    classDef startend fill:#f9f,stroke:#333,stroke-width:2px;
    classDef process fill:#e1f5fe,stroke:#0277bd,stroke-width:2px;
    classDef decision fill:#fff9c4,stroke:#fbc02d,stroke-width:2px;
    classDef subfunction fill:#e0f2f1,stroke:#00695c,stroke-width:1px,stroke-dasharray: 5 5;

    Start(("程序启动<br>main")):::startend --> Init["初始化 ThunderFighter<br>Constructor"]:::process
    Init --> LoadHS["加载最高分<br>LoadHighScore"]:::subfunction
    Init --> InitEnemies["生成初始敌人<br>Make_enermy"]:::subfunction
    
    Init --> MainLoop{"主循环检查<br>IsRunning?"}:::decision
    
    %% 游戏主循环逻辑
    MainLoop -- True --> Run["进入 Run()"]:::process
    Run --> GameLoop{"单局循环<br>running_ && !ShouldExit"}:::decision
    
    %% DrawFrame 内部详细逻辑
    GameLoop -- True --> DrawFrame["渲染帧<br>DrawFrame"]:::process
    
    subgraph DrawFrame_Logic [DrawFrame 内部逻辑]
        direction TB
        CheckPause{"按下 P 键?"}:::decision
        CheckPause -- Yes --> TogglePause["切换 is_paused_ 状态<br>修正时间"]:::process
        CheckPause -- No --> IsPaused{"当前暂停?<br>is_paused_"}:::decision
        
        IsPaused -- Yes --> RenderPause["绘制 PAUSED 文字"]:::process
        
        IsPaused -- No --> LogicUpdate["逻辑更新"]:::process
        LogicUpdate --> Funcs["功能函数调用序列"]:::subfunction
        
        subgraph LogicFunctions [核心逻辑调用顺序]
            direction TB
            F1["MoveEnemies<br>移动敌人"]
            F2["SpawnEnemiesFromPending<br>生成新怪"]
            F3["Level & score<br>难度与分数更新"]
            F4["Input Check<br>WASD/Space/Cheats"]
            F5["CheckPlayerCollision<br>玩家碰撞检测"]
            F6["UpdateBullets<br>子弹移动与碰撞"]
            
            F1 --> F2 --> F3 --> F4 --> F5 --> F6
        end
        
        LogicUpdate --> LogicFunctions
        LogicFunctions --> RenderGame["渲染游戏画面"]:::process
    end
    
    DrawFrame --> Sleep["休眠 16ms<br>控制帧率"]:::process
    Sleep --> GameLoop
    
    %% 游戏结束逻辑
    GameLoop -- False --> GameOverProcess["结算流程"]:::process
    GameOverProcess --> SaveData["保存数据<br>SaveHighScore"]:::subfunction
    
    GameOverProcess --> GameOverScreen{"结算界面循环"}:::decision
    
    GameOverScreen -- "按 R 键" --> Reset["重置游戏<br>ResetGame"]:::process
    Reset --> ReturnRun["退出 Run 函数<br>返回 MainLoop"]
    
    GameOverScreen -- "按 Q 键" --> QuitSet["设置 running_=false"]:::process
    QuitSet --> ReturnRun
    
    ReturnRun --> MainLoop
    
    MainLoop -- False --> End(("程序结束")):::startend
