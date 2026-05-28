#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>

namespace ConsoleUtils
{

void ClearScreen()
{
std::cout << "\033[2J\033[H";
std::cout << "\033[3J";
std::cout << "\033[?1049h\033[r\033[50;1H";
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

} // namespace ConsoleUtils
