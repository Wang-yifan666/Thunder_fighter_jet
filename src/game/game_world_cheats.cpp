// GameWorld 的作弊逻辑。内容来自重构前的 cheats.cpp，
// 仅把类名改为 GameWorld 并移入 thunder 命名空间。

#include "game/game_world.h"
#include "game/game_config.h"

namespace thunder
{

void GameWorld::cheats_kills()
{
	enemies_.clear();
	pending_enemies_.clear();
	remaining_rows_in_batch_ = 0;
}

void GameWorld::cheats_life()
{
	life_number++;
}

void GameWorld::cheats_invincible()
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

void GameWorld::cheats_addscore()
{
	score_ += GameConfig::kCheatAddScore; //每次加分
}

void GameWorld::cheats_addbullters()
{
	bullets_remaining_ += GameConfig::kCheatAddBullets; //每次加子弹
}

void GameWorld::cheats_godlike()
{
	cheats_invincible();
	life_number = GameConfig::kCheatGodlikeLife;
	bullets_remaining_ = GameConfig::kCheatGodlikeBullets;
}

void GameWorld::kill_self()
{
	life_number = 0;
}

} // namespace thunder
