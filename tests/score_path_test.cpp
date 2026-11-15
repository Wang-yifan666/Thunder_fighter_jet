// 存档位置解析与文件写入格式的测试。
//
// 重点覆盖"装到不可写目录后成绩怎么办"这个真实场景：
// 便携模式优先，exe 目录不可写时退到用户数据目录。

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "io/file_score_store.h"
#include "io/score_path.h"

namespace
{

namespace fs = std::filesystem;

/// 用固定名字的临时目录，测试结束自动清理。
class TempDir
{
public:
	explicit TempDir(const std::string& name)
	{
		path_ = fs::temp_directory_path() / ("naidan_test_" + name);
		std::error_code ec;
		fs::remove_all(path_, ec);
		fs::create_directories(path_, ec);
	}

	~TempDir()
	{
		std::error_code ec;
		fs::remove_all(path_, ec);
	}

	TempDir(const TempDir&) = delete;
	TempDir& operator=(const TempDir&) = delete;

	fs::path path() const
	{
		return path_;
	}

	/// 带结尾分隔符的字符串形式（ResolveScorePath 的约定）
	std::string str() const
	{
		std::string s = path_.string();
		if(!s.empty() && s.back() != '\\' && s.back() != '/')
			s.push_back(fs::path::preferred_separator);
		return s;
	}

private:
	fs::path path_;
};

/// 读写环境变量的简易守卫，析构时恢复原值。
class EnvGuard
{
public:
	EnvGuard(const char* name, const char* value): name_(name)
	{
		if(const char* old = std::getenv(name))
		{
			had_value_ = true;
			old_value_ = old;
		}
		SetValue(value);
	}

	~EnvGuard()
	{
		SetValue(had_value_ ? old_value_.c_str() : nullptr);
	}

	EnvGuard(const EnvGuard&) = delete;
	EnvGuard& operator=(const EnvGuard&) = delete;

private:
	void SetValue(const char* value)
	{
#ifdef _WIN32
		_putenv_s(name_.c_str(), value != nullptr ? value : "");
#else
		if(value != nullptr)
			setenv(name_.c_str(), value, 1);
		else
			unsetenv(name_.c_str());
#endif
	}

	std::string name_;
	std::string old_value_;
	bool had_value_ = false;
};

std::vector<char> ReadAllBytes(const std::string& path)
{
	std::ifstream in(path, std::ios::binary);
	return std::vector<char>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

} // namespace

TEST(ScorePath, PortableWhenExeDirIsWritable)
{
	TempDir exe_dir("portable");
	// 清空覆盖变量，确保走的是"便携优先"这条路径
	EnvGuard guard("NAIDAN_DATA_DIR", "");

	const std::string resolved = thunder::ResolveScorePath(exe_dir.str());
	EXPECT_EQ(fs::path(resolved), exe_dir.path() / "highscore.txt");
}

TEST(ScorePath, FallsBackToUserDataWhenExeDirMissing)
{
	TempDir data_dir("userdata");
	TempDir placeholder("missing_exe");
	// 指向一个不存在的目录，模拟"不可写的安装位置"
	const fs::path missing = placeholder.path() / "not_installed_here";

	EnvGuard guard("NAIDAN_DATA_DIR", data_dir.path().string().c_str());

	const std::string resolved = thunder::ResolveScorePath(missing.string() + "\\");
	EXPECT_EQ(fs::path(resolved), data_dir.path() / "highscore.txt");
}

TEST(ScorePath, ExplicitEnvOverrideWins)
{
	TempDir data_dir("override");
	TempDir exe_dir("override_exe");

	EnvGuard guard("NAIDAN_DATA_DIR", data_dir.path().string().c_str());

	// 即使 exe 目录可写，显式指定的数据目录也应该优先
	const std::string resolved = thunder::ResolveScorePath(exe_dir.str());
	EXPECT_EQ(fs::path(resolved), data_dir.path() / "highscore.txt");
}

TEST(ScorePath, KeepsLegacyParentFileWhenExeDirUnwritable)
{
	TempDir root("legacy");
	const fs::path legacy_file = root.path() / "highscore.txt";
	{
		std::ofstream out(legacy_file, std::ios::binary);
		out << "123\n";
	}

	// exe 目录不存在 -> 不可写；上一级有存档 -> 用它
	const fs::path missing_exe = root.path() / "build";
	EnvGuard guard("NAIDAN_DATA_DIR", "");

	const std::string resolved = thunder::ResolveScorePath(missing_exe.string() + "\\");
	EXPECT_EQ(fs::path(resolved), missing_exe / ".." / "highscore.txt");
}

TEST(FileScoreStore, WritesLfNotCrLf)
{
	TempDir dir("writer");
	const fs::path file = dir.path() / "highscore.txt";

	thunder::FileScoreStore store(file.string());
	store.Save({300, 200, 100});

	const std::vector<char> bytes = ReadAllBytes(file.string());
	ASSERT_FALSE(bytes.empty());

	for(char c: bytes)
		EXPECT_NE(c, '\r') << "不应写入 CR，否则 Windows 下会变成 CRLF";

	const std::string content(bytes.begin(), bytes.end());
	EXPECT_EQ(content, "300\n200\n100\n");
}

TEST(FileScoreStore, LoadCreatesFileWithZero)
{
	TempDir dir("create");
	const fs::path file = dir.path() / "highscore.txt";
	ASSERT_FALSE(fs::exists(file));

	thunder::FileScoreStore store(file.string());
	const std::vector<int> loaded = store.Load();

	ASSERT_EQ(loaded.size(), 1u);
	EXPECT_EQ(loaded[0], 0);
	EXPECT_TRUE(fs::exists(file));
}

TEST(FileScoreStore, RoundTripsScores)
{
	TempDir dir("roundtrip");
	const fs::path file = dir.path() / "highscore.txt";

	thunder::FileScoreStore store(file.string());
	store.Save({900, 100, 500});

	const std::vector<int> loaded = store.Load();
	ASSERT_EQ(loaded.size(), 3u);
	EXPECT_EQ(loaded[0], 900);
	EXPECT_EQ(loaded[1], 100);
	EXPECT_EQ(loaded[2], 500);
}
