#pragma once

#include "game/game_config.h"
#include "game/game_types.h"

struct Enemy
{
	int x;
	int y;
	int width;
	int height;
	int speed;
	bool alive;

	// 战斗属性：普通小怪 1 血，精英 2 血；带盾时先破盾。
	EnemyKind kind;
	int hp;
	bool has_shield;

	// 护盾刚破时的无敌帧：> 0 时子弹穿过（不消耗）、敌机闪烁。
	int invuln_frames;

	// 上一帧的坐标：敌机一次可以下移 speed（最多 3）格，
	// 只比末位置会漏掉"子弹和敌机中途擦身而过"，命中判定要用这两个值做扫掠。
	int prev_x;
	int prev_y;

	Enemy(int _x = 0,                             //
	      int _y = 0,                             //
	      int _speed = 1,                         //
	      int _width = GameConfig::kEnemyWidth,   //
	      int _height = GameConfig::kEnemyHeight, //
	      EnemyKind _kind = EnemyKind::Normal,    //
	      bool _has_shield = false,               //
	      int _invuln_frames = 0                  //
	      ):
	x(_x),                                //
	y(_y),                                //
	width(_width),                        //
	height(_height),                      //
	speed(_speed),                        //
	alive(true),                          //
	kind(_kind),                          //
	hp(_kind == EnemyKind::Elite ? GameConfig::kEliteHp : 1), //
	has_shield(_has_shield),              //
	invuln_frames(_invuln_frames),        //
	prev_x(_x),                           //
	prev_y(_y)                            //
	{
	}

	/// 精英机：4x3 格、kEliteHp 点血，可带护盾。
	static Enemy Elite(int _x, int _y, int _speed, bool _has_shield)
	{
		return Enemy(_x, _y, _speed, GameConfig::kEliteWidth, GameConfig::kEliteHeight,
		             EnemyKind::Elite, _has_shield);
	}
};
