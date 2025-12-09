#include <windows.h>
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