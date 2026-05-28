#include <windows.h>

namespace ConsoleInput
{

bool IsKeyDown(int vk_code)
{
	return (GetAsyncKeyState(vk_code) & 0x8000) != 0;
}

} // namespace ConsoleInput