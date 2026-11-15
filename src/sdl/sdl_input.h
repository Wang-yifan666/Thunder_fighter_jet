#pragma once

#include <SDL3/SDL.h>

#include "game/input_state.h"

namespace SdlInput
{

/// 把 SDL 键盘事件转换为 thunder::RawInput。
/// down 记录当前按住状态；pressed_events 记录按下事件，
/// 在第一次被逻辑步消费后由 ConsumePressedEvents() 清空。
class State
{
public:
	void HandleEvent(const SDL_Event& event);

	thunder::RawInput Build() const
	{
		return raw_;
	}

	/// 逻辑步消费完按下事件后调用
	void ConsumePressedEvents()
	{
		raw_.pressed_events.reset();
	}

private:
	void SetKey(SDL_Scancode scancode, bool down);

	thunder::RawInput raw_;
};

} // namespace SdlInput
