#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
//#include <string>
#include <chrono>
#include <vector>
//#include <random>

#include "enemy.h"

class ThunderFighter
{
public:
	ThunderFighter();
	~ThunderFighter() = default;

	void Run(); //主循环

private:
	//===== 高层游戏逻辑 =====
	void DrawFrame();        //每帧更新 + 渲染
	void Level();            //等级 / 难度管理
	void score();            //分数随时间增加
	void Life();             //生命相关逻辑
	bool ShouldExit() const; //检查是否退出游戏

	//===== 敌人相关 =====
	void Make_enermy();             //生成pending敌人
	void Clear_Current_Enemies();   //清空当前enemies_
	void SpawnEnemiesFromPending(); //从pending刷一批敌人到屏幕
	bool IsOverlapping(const Enemy& a,
	                   const Enemy& b); //检查两个敌人是否重叠
	bool WillOverlap(const Enemy& cur,
	                 const std::vector<Enemy>& enemies, int new_x,
	                 int new_y); //预测移动后是否重叠
	void MoveEnemies(std::vector<Enemy>& active_enemies, int level,
	                 int screen_width,
	                 int screen_height); //敌人实际移动

	// ========= 新增：玩家碰撞检测 =========
	void CheckPlayerCollision(); //玩家与敌人碰撞检测

	//===== 控制台 / 输出工具 =====
	void ClearScreen() const;   //清屏
	void HideCursor() const;    //隐藏光标
	void cheats_kills();        //作弊：秒杀所有敌人
	void cheats_life();         //作弊：加血
	void LoadHighScore();       //加载最高分
	void SaveHighScore() const; //保存最高分

	//===== 状态：核心游戏数据 =====
	// 按照构造函数初始化列表的顺序声明，避免编译器关于“初始化顺序不同”的警告
	int frame_count_; //总帧数
	bool running_;    //是否在运行
	int fps_counter_; //每秒统计用
	int current_fps_; //最近一次计算出的 FPS

	int player_x_; //玩家位置
	int player_y_;
	int life_number; //剩余生命数

	int level_; //当前关卡/等级

	std::chrono::steady_clock::time_point start_time_; //游戏开始时间

	int enemy_move_counter_;      //敌人移动计数器
	int enemy_move_interval_;     //敌人移动间隔
	int remaining_rows_in_batch_; //当前刷怪批次剩余行数

	int score_;               //分数
	int highest_score_;       //最高分
	bool top_row_clear_;      //顶部是否空（用于刷怪）
	int last_score_time_ = 0; //上次加分的时间点（秒）

	//===== 状态：辅助计时 / 敌人容器 =====
	std::chrono::steady_clock::time_point
	    last_fps_time_;          //上次 FPS 统计时间
	bool is_invincible_ = false; //当前是否处于无敌
	std::chrono::steady_clock::time_point
	    invincible_until_; // 无敌结束时间

	std::vector<Enemy> enemies_;         //屏幕上的敌人
	std::vector<Enemy> pending_enemies_; //待生成敌人队列
	int alive_enemies_count = 0;         //存活敌人数

	int elapsed_seconds_ = 0; //游戏已运行秒数
};