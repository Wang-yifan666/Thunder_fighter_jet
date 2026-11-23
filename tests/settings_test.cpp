// 设置文件读写：round-trip、缺失文件、脏数据都要回退到默认值而不是崩。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "game/settings_store.h"
#include "io/file_settings_store.h"

namespace
{

std::string TempPath(const char* name)
{
	const auto dir = std::filesystem::temp_directory_path() / "naidan_settings_test";
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
	return (dir / name).string();
}

} // namespace

TEST(Settings, MissingFileGivesDefaults)
{
	const std::string path = TempPath("missing.txt");
	std::error_code ec;
	std::filesystem::remove(path, ec);

	thunder::FileSettingsStore store(path);
	const thunder::PlayerSettings settings = store.Load();

	EXPECT_FALSE(settings.tutorial_done);
	EXPECT_EQ(settings.skin_id, 0);
}

TEST(Settings, SaveThenLoadRoundTrips)
{
	const std::string path = TempPath("roundtrip.txt");

	thunder::FileSettingsStore store(path);
	thunder::PlayerSettings out;
	out.tutorial_done = true;
	out.skin_id = 3;
	store.Save(out);

	thunder::FileSettingsStore reloaded(path);
	const thunder::PlayerSettings in = reloaded.Load();
	EXPECT_TRUE(in.tutorial_done);
	EXPECT_EQ(in.skin_id, 3);
}

TEST(Settings, GarbageAndCrlfAreTolerated)
{
	const std::string path = TempPath("garbage.txt");
	{
		std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);
		file << "tutorial_done=1\r\n";
		file << "skin=abc\r\n";     // 坏值：当作没设置
		file << "unknown=42\r\n";   // 不认识的键：忽略
		file << "no_equals_sign\r\n";
	}

	thunder::FileSettingsStore store(path);
	const thunder::PlayerSettings settings = store.Load();

	EXPECT_TRUE(settings.tutorial_done) << "CRLF 不该导致整行作废";
	EXPECT_EQ(settings.skin_id, 0) << "坏值回退到默认装扮";
}

TEST(Settings, MemoryStoreKeepsValue)
{
	thunder::MemorySettingsStore store;
	thunder::PlayerSettings settings;
	settings.tutorial_done = true;
	store.Save(settings);

	EXPECT_TRUE(store.Load().tutorial_done);
	EXPECT_EQ(store.Load().skin_id, 0);
}
