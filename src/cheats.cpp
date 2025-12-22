#include <windows.h>
#include <climits>
#include "def.h"

//#include <algorithm>

namespace
{
//constexpr int cheat_invincible_;
}

void ThunderFighter::cheats_kills()
{
	enemies_.clear();
	pending_enemies_.clear();
	alive_enemies_count = 0;
	remaining_rows_in_batch_ = 0;
}

void ThunderFighter::cheats_life()
{
	life_number++;
}

void ThunderFighter::cheats_invincible()
{
	cheat_invincible_ = !cheat_invincible_;

	if(cheat_invincible_)
	{
		is_invincible_ = true;
	}
	else
	{
		is_invincible_ = false;
	}
}

void ThunderFighter::cheats_addscore()
{
	score_ += 1000; //每次加1000分
}

void ThunderFighter::cheats_addbullters()
{
	bullets_remaining_ += 100; //每次加100发子弹
}


void ThunderFighter::cheats_godlike()
{
	cheats_invincible();
	life_number = INT_MAX;
	bullets_remaining_ = INT_MAX;
}