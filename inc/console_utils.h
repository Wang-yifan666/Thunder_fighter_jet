#pragma once

#include <string>

namespace ConsoleUtils
{

void ClearScreen();
void ClearTerminal();
void HideCursor();
void ShowCursor();
void SleepMs(int ms);
void EnableUTF8();

/// 当前可执行文件所在目录（以路径分隔符结尾）
std::string ExecutableDirectory();

} // namespace ConsoleUtils
