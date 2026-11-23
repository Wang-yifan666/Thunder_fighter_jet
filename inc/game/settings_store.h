#pragma once

namespace thunder
{

/// 跨启动保存的玩家设置。
/// 目前只有两件事：教程是否通关（决定装扮是否解锁）、当前选中的装扮。
struct PlayerSettings
{
	bool tutorial_done = false;
	int skin_id = 0;
};

/// 设置持久化接口（纯逻辑：不接触文件系统）。
/// 具体实现由 thunder_io 提供（FileSettingsStore），测试用 MemorySettingsStore。
class SettingsStore
{
public:
	virtual ~SettingsStore() = default;

	virtual PlayerSettings Load() = 0;
	virtual void Save(const PlayerSettings& settings) = 0;
};

class MemorySettingsStore: public SettingsStore
{
public:
	PlayerSettings Load() override
	{
		return settings_;
	}

	void Save(const PlayerSettings& settings) override
	{
		settings_ = settings;
	}

private:
	PlayerSettings settings_;
};

} // namespace thunder
