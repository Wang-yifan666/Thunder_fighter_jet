#include "render_console.h"

using namespace ftxui;

namespace RenderConsole
{

Element CellToElement(char c, bool inv, bool paused, int y, int height)
{
	if (c == '*' || c == '0')
	{
		auto col = inv ? Color::GreenLight : Color::YellowLight;
		return text(std::string(1, c)) | bold | color(col);
	}
	if (c == '$' || c == 'o')
		return text(std::string(1, c)) | bold | color(Color::RedLight);
	if (c == '|')
		return text("|") | bold | color(Color::Magenta);
	if (paused && y == height / 2 && c != ' ')
	{
		return text(std::string(1, c)) | bold | color(Color::White);
	}
	return text(" ");
}

std::string LifeString(int life, int max_show)
{
	std::string life_str = "剩余生命: ";
	for (int i = 1; i <= life; i++)
	{
		if (i >= max_show)
		{
			life_str += " ++ ";
			break;
		}
		else
		{
			life_str += "<> ";
		}
	}
	return life_str;
}

} // namespace RenderConsole