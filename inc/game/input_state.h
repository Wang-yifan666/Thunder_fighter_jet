#pragma once

#include <array>
#include <bitset>
#include <cstddef>

namespace thunder
{

enum class Key
{
	Up,
	Down,
	Left,
	Right,
	Fire,
	Pause,
	Quit,
	Back,
	Restart,
	Escape,
	Digit1,
	Digit2,
	Digit3,
	Digit4,
	Digit5,
	Digit6,
	Digit7,
	Digit8,
	Digit9,
	Count
};

constexpr std::size_t kKeyCount = static_cast<std::size_t>(Key::Count);

using KeyBits = std::bitset<kKeyCount>;

inline std::size_t KeyIndex(Key key)
{
	return static_cast<std::size_t>(key);
}

/// 前端采集到的原始输入。
/// - down：当前按住的键。
/// - pressed_events：本帧发生的按下事件。事件式后端（如 SDL）必须填写，
///   否则"按下后很快释放"会在两次采样之间被吞掉；轮询式后端留空即可，
///   由 InputTracker 根据 down 的变化补出边沿。
struct RawInput
{
	KeyBits down;
	KeyBits pressed_events;
};

struct InputState
{
	KeyBits down;
	KeyBits pressed;

	bool Down(Key key) const { return down.test(KeyIndex(key)); }
	bool Pressed(Key key) const { return pressed.test(KeyIndex(key)); }
};

/// 把 RawInput 转换为带按下沿的 InputState。
/// 每个流程对象（终端前端 / 将来的 GameSession）各自持有唯一实例，不做全局共享。
class InputTracker
{
public:
	InputState Update(const RawInput& raw)
	{
		InputState state;
		state.down = raw.down;
		state.pressed = raw.pressed_events | (raw.down & ~previous_down_);
		previous_down_ = raw.down;
		return state;
	}

private:
	KeyBits previous_down_;
};

/// GameWorld 需要的游戏内输入（不含菜单 / 暂停 / 退出等流程按键）。
struct GameInput
{
	bool left = false;
	bool right = false;
	bool up = false;
	bool down = false;
	bool fire = false;
	std::array<bool, 10> digit{};

	static GameInput FromInput(const InputState& input)
	{
		GameInput out;
		out.left = input.Down(Key::Left);
		out.right = input.Down(Key::Right);
		out.up = input.Down(Key::Up);
		out.down = input.Down(Key::Down);
		out.fire = input.Down(Key::Fire);
		out.digit[1] = input.Down(Key::Digit1);
		out.digit[2] = input.Down(Key::Digit2);
		out.digit[3] = input.Down(Key::Digit3);
		out.digit[4] = input.Down(Key::Digit4);
		out.digit[5] = input.Down(Key::Digit5);
		out.digit[6] = input.Down(Key::Digit6);
		out.digit[7] = input.Down(Key::Digit7);
		out.digit[8] = input.Down(Key::Digit8);
		out.digit[9] = input.Down(Key::Digit9);
		return out;
	}
};

} // namespace thunder
