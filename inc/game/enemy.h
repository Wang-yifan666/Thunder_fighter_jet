#pragma once

#include "game/game_config.h"

struct Enemy
{
	int x;
	int y;
	int width;
	int height;
	int speed;
	bool alive;

	Enemy(int _x = 0,                            //
	      int _y = 0,                            //
	      int _speed = 1,                        //
	      int _width = GameConfig::kEnemyWidth,  //
	      int _height = GameConfig::kEnemyHeight //
	      ):
	x(_x),           //
	y(_y),           //
	width(_width),   //
	height(_height), //
	speed(_speed),   //
	alive(true)      //
	{
	}
};
