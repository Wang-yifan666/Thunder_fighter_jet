#define FTXUI_MICROSOFT_TERMINAL_FALLBACK 1
#include "def.h"

int main()
{
	ThunderFighter game;

	while(game.IsRunning())
	{
		game.Run();
	}

	return 0;
}
