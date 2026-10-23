#include "def.h"
#include "console_utils.h"

int main()
{
	ConsoleUtils::EnableUTF8();

	ThunderFighter game;

	while(game.IsRunning())
	{
		game.Run();
	}

	ConsoleUtils::ShowCursor();

	return 0;
}
