#define FTXUI_MICROSOFT_TERMINAL_FALLBACK 1
#include <windows.h>
#include "def.h"

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	ThunderFighter game;

	while(game.IsRunning())
	{
		game.Run();
	}

	return 0;
}
