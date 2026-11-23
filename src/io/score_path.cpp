#include "io/score_path.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace thunder
{
namespace
{

namespace fs = std::filesystem;

/// 目录可写探测：在目标目录里实际创建一次文件。
/// 只看权限位并不可靠（UAC 虚拟化、只读介质都会让"看起来有权限"落空）。
bool DirectoryWritable(const fs::path& dir)
{
	std::error_code ec;
	if(!fs::is_directory(dir, ec))
		return false;

	const fs::path probe = dir / ".naidan_write_probe";
	{
		std::ofstream out(probe, std::ios::binary | std::ios::app);
		if(!out)
			return false;
	}
	fs::remove(probe, ec);
	return true;
}

/// 用户数据目录：%APPDATA%\Naidan > %USERPROFILE%\.naidan > 临时目录。
fs::path UserDataDir()
{
	if(const char* appdata = std::getenv("APPDATA"))
	{
		if(appdata[0] != '\0')
			return fs::path(appdata) / "Naidan";
	}

	if(const char* home = std::getenv("USERPROFILE"))
	{
		if(home[0] != '\0')
			return fs::path(home) / ".naidan";
	}

	std::error_code ec;
	return fs::temp_directory_path(ec) / "Naidan";
}

} // namespace

std::string ResolveDataPath(const std::string& executable_dir, const char* file_name)
{
	const fs::path exe_dir(executable_dir);
	const fs::path local = exe_dir / file_name;
	std::error_code ec;

	// 显式指定数据目录时优先（测试用，也方便用户强制把存档放到别处）
	if(const char* override_dir = std::getenv("NAIDAN_DATA_DIR"))
	{
		if(override_dir[0] != '\0')
		{
			const fs::path dir(override_dir);
			fs::create_directories(dir, ec);
			return (dir / file_name).string();
		}
	}

	// 便携模式：exe 同目录可写就写在那里
	if(DirectoryWritable(exe_dir))
		return local.string();

	// 兼容旧布局：上一级目录已有同名文件就用它
	const fs::path legacy = exe_dir / ".." / file_name;
	if(fs::exists(legacy, ec))
		return legacy.string();

	// 装到了不可写的位置（例如 Program Files）：退到用户数据目录
	const fs::path user_dir = UserDataDir();
	fs::create_directories(user_dir, ec);
	return (user_dir / file_name).string();
}

std::string ResolveScorePath(const std::string& executable_dir)
{
	return ResolveDataPath(executable_dir, "highscore.txt");
}

std::string ResolveSettingsPath(const std::string& executable_dir)
{
	return ResolveDataPath(executable_dir, "settings.txt");
}

} // namespace thunder
