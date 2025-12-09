#pragma once

struct Enemy
{
	int x;
	int y;
	int width;
	int height;
	int speed;
	bool alive;

	Enemy(int _x = 0, int _y = 0, int _speed = 1, int _width = 3,
	      int _height = 1):
	x(_x), y(_y), width(_width), height(_height), speed(_speed),
	alive(true)
	{
	}
};
