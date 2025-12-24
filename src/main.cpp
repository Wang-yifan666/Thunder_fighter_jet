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

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); //解决有时终端光标消失
	CONSOLE_CURSOR_INFO ci{1, TRUE};
	SetConsoleCursorInfo(h, &ci);

	return 0;
}
