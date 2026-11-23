#include "io/file_settings_store.h"

#include <fstream>
#include <string>

namespace thunder
{

FileSettingsStore::FileSettingsStore(std::string path): path_(std::move(path))
{
}

PlayerSettings FileSettingsStore::Load()
{
	PlayerSettings settings;

	std::ifstream in(path_, std::ios::binary);
	if(!in)
		return settings; // 文件不存在：全部用默认值

	std::string line;
	while(std::getline(in, line))
	{
		// 兼容 CRLF：Windows 记事本改过之后行尾会带 \r
		if(!line.empty() && line.back() == '\r')
			line.pop_back();

		const std::size_t eq = line.find('=');
		if(eq == std::string::npos)
			continue;

		const std::string key = line.substr(0, eq);
		const std::string value = line.substr(eq + 1);

		if(key == "tutorial_done")
			settings.tutorial_done = (value == "1");
		else if(key == "skin")
		{
			try
			{
				settings.skin_id = std::stoi(value);
			}
			catch(...)
			{
				settings.skin_id = 0; // 坏值就当没设置过
			}
		}
	}

	return settings;
}

void FileSettingsStore::Save(const PlayerSettings& settings)
{
	std::ofstream out(path_, std::ios::binary | std::ios::trunc);
	if(!out)
		return; // 写不进去就算了，不该影响对局

	out << "tutorial_done=" << (settings.tutorial_done ? 1 : 0) << '\n';
	out << "skin=" << settings.skin_id << '\n';
}

} // namespace thunder
