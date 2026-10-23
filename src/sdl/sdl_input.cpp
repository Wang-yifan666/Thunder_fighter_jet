// SDL 事件 → RawInput 映射。键位与终端版保持一致。

#include "sdl_input.h"

namespace
{

bool MapScancode(SDL_Scancode scancode, thunder::Key& out)
{
	switch(scancode)
	{
	case SDL_SCANCODE_W:
		out = thunder::Key::Up;
		return true;
	case SDL_SCANCODE_S:
		out = thunder::Key::Down;
		return true;
	case SDL_SCANCODE_A:
		out = thunder::Key::Left;
		return true;
	case SDL_SCANCODE_D:
		out = thunder::Key::Right;
		return true;
	case SDL_SCANCODE_SPACE:
		out = thunder::Key::Fire;
		return true;
	case SDL_SCANCODE_P:
		out = thunder::Key::Pause;
		return true;
	case SDL_SCANCODE_Q:
		out = thunder::Key::Quit;
		return true;
	case SDL_SCANCODE_B:
		out = thunder::Key::Back;
		return true;
	case SDL_SCANCODE_R:
		out = thunder::Key::Restart;
		return true;
	case SDL_SCANCODE_ESCAPE:
		out = thunder::Key::Escape;
		return true;
	case SDL_SCANCODE_1:
		out = thunder::Key::Digit1;
		return true;
	case SDL_SCANCODE_2:
		out = thunder::Key::Digit2;
		return true;
	case SDL_SCANCODE_3:
		out = thunder::Key::Digit3;
		return true;
	case SDL_SCANCODE_4:
		out = thunder::Key::Digit4;
		return true;
	case SDL_SCANCODE_5:
		out = thunder::Key::Digit5;
		return true;
	case SDL_SCANCODE_6:
		out = thunder::Key::Digit6;
		return true;
	case SDL_SCANCODE_7:
		out = thunder::Key::Digit7;
		return true;
	case SDL_SCANCODE_8:
		out = thunder::Key::Digit8;
		return true;
	case SDL_SCANCODE_9:
		out = thunder::Key::Digit9;
		return true;
	default:
		return false;
	}
}

} // namespace

namespace SdlInput
{

void State::SetKey(SDL_Scancode scancode, bool down)
{
	thunder::Key key;

	if(!MapScancode(scancode, key))
		return;

	const std::size_t index = thunder::KeyIndex(key);

	if(down)
	{
		raw_.down.set(index);
		raw_.pressed_events.set(index);
	}
	else
	{
		raw_.down.reset(index);
	}
}

void State::HandleEvent(const SDL_Event& event)
{
	if(event.type == SDL_EVENT_KEY_DOWN)
	{
		if(event.key.repeat)
			return;
		SetKey(event.key.scancode, true);
	}
	else if(event.type == SDL_EVENT_KEY_UP)
	{
		SetKey(event.key.scancode, false);
	}
}

} // namespace SdlInput
