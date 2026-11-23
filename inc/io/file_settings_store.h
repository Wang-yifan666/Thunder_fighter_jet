#pragma once

#include <string>

#include "game/settings_store.h"

namespace thunder
{

/// 基于文件的设置存储：`key=value` 纯文本，一行一项。
///
/// 只认两个键：
///   tutorial_done=0/1   教程是否通关（决定装扮解锁）
///   skin=<id>           当前装扮
/// 文件缺失、损坏或键不认得时一律回退到默认值——设置读不出来不该让游戏起不来。
class FileSettingsStore: public SettingsStore
{
public:
	explicit FileSettingsStore(std::string path = "settings.txt");

	PlayerSettings Load() override;
	void Save(const PlayerSettings& settings) override;

private:
	std::string path_;
};

} // namespace thunder
