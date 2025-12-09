#include <windows.h>
#include "def.h"

//#include <algorithm>

void ThunderFighter::cheats_kills()
{
	enemies_.clear();
	pending_enemies_.clear();
	alive_enemies_count = 0;
}

void ThunderFighter::cheats_life()
{
	life_number++;
}