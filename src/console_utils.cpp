#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "game/game_config.h"

namespace ConsoleUtils
{

void ClearScreen()
{
std::cout << "\033[2J\033[H";
std::cout << "\033[3J";
// 光标移到底部，避免窗口高度变化时出现滚动
std::cout << "\033[?1049h\033[r\033[" << GameConfig::kWindowHeight << ";1H";
std::cout << std::flush;
}

void ClearTerminal()
{
system("cls");
}

void HideCursor()
{
HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_CURSOR_INFO ci{1, FALSE};
SetConsoleCursorInfo(h, &ci);
}

void ShowCursor()
{
HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_CURSOR_INFO ci{1, TRUE};
SetConsoleCursorInfo(h, &ci);
}

void SleepMs(int ms)
{
std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void EnableUTF8()
{
SetConsoleOutputCP(CP_UTF8);
SetConsoleCP(CP_UTF8);
}

std::string ExecutableDirectory()
{
char buffer[MAX_PATH] = {};
DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
if(length == 0)
	return ".\\";

std::string path(buffer, length);
const std::string::size_type pos = path.find_last_of("\\/");
if(pos == std::string::npos)
	return ".\\";

return path.substr(0, pos + 1);
}

} // namespace ConsoleUtils
