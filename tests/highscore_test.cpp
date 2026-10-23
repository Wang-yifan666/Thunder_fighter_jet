// B0 基线测试：只看 HighScore（当前已与平台无关）。
// 这些用例锁定现有排行榜规则，后续搬迁到 thunder_logic 时不得改动期望值。

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "game/highscore.h"
#include "io/file_score_store.h"

namespace
{

/// 每个用例独享一个临时排行榜文件，避免污染仓库里的 highscore.txt。
class TempScoreFile
{
public:
	TempScoreFile()
	{
		static const auto seed =
		    std::chrono::steady_clock::now().time_since_epoch().count();
		static int counter = 0;

		const auto dir = std::filesystem::temp_directory_path()
		    / "thunder_highscore_tests";
		std::filesystem::create_directories(dir);

		path_ = dir
		    / ("hs_" + std::to_string(seed) + "_"
		       + std::to_string(counter++) + ".txt");

		std::error_code ec;
		std::filesystem::remove(path_, ec);
	}

	~TempScoreFile()
	{
		std::error_code ec;
		std::filesystem::remove(path_, ec);
	}

	TempScoreFile(const TempScoreFile&) = delete;
	TempScoreFile& operator=(const TempScoreFile&) = delete;

	std::string Path() const { return path_.string(); }

	bool Exists() const { return std::filesystem::exists(path_); }

	void Write(const std::string& content) const
	{
		std::ofstream fout(path_);
		fout << content;
	}

private:
	std::filesystem::path path_;
};

} // namespace

// 文件不存在时应创建它并写入 0（现有行为）
TEST(HighScore, LoadCreatesZeroWhenFileMissing)
{
	TempScoreFile file;

	thunder::FileScoreStore store(file.Path());
	thunder::HighScore hs(store);
	hs.Load();

	ASSERT_TRUE(file.Exists());
	ASSERT_EQ(hs.Scores().size(), 1u);
	EXPECT_EQ(hs.Scores().front(), 0);
}

// 现状语义（锁定，Stage 2 不改）：Load 先读满 max_keep 行就停，再排序。
// 也就是说它取的是"文件前 5 行"，而不是"文件里的前 5 名"。
// 正常游玩路径永远不会触发这种差异：Submit 总是把已排序的前 5 名写回文件。
TEST(HighScore, LoadKeepsFirstFiveLinesThenSorts)
{
	TempScoreFile file;
	file.Write("30\n90\n10\n70\n50\n20\n");

	thunder::FileScoreStore store(file.Path());
	thunder::HighScore hs(store);
	hs.Load();

	// 第 6 行的 20 被丢弃（尽管它比 10 大）
	const std::vector<int> expected{90, 70, 50, 30, 10};
	EXPECT_EQ(hs.Scores(), expected);
}

// 正常路径：文件本身已是降序，读取后保持前 5 名
TEST(HighScore, LoadKeepsTopFiveFromSortedFile)
{
	TempScoreFile file;
	file.Write("90\n70\n50\n30\n20\n10\n");

	thunder::FileScoreStore store(file.Path());
	thunder::HighScore hs(store);
	hs.Load();

	const std::vector<int> expected{90, 70, 50, 30, 20};
	EXPECT_EQ(hs.Scores(), expected);
}

// Submit 后排序、截断，并且落盘可被重新读回
TEST(HighScore, SubmitKeepsTopFiveAndPersists)
{
	TempScoreFile file;

	thunder::FileScoreStore store(file.Path());
	thunder::HighScore hs(store);
	hs.Load();

	for(int score: {30, 10, 50, 20, 40, 5})
		hs.Submit(score);

	const std::vector<int> expected{50, 40, 30, 20, 10};
	EXPECT_EQ(hs.Scores(), expected);

	thunder::FileScoreStore reload_store(file.Path());
	thunder::HighScore reloaded(reload_store);
	reloaded.Load();
	EXPECT_EQ(reloaded.Scores(), expected);
}

// max_keep 参数必须生效
TEST(HighScore, SubmitHonorsMaxKeep)
{
	TempScoreFile file;

	thunder::FileScoreStore store(file.Path());
	thunder::HighScore hs(store, 3);
	hs.Load();

	for(int score: {1, 2, 3, 4})
		hs.Submit(score);

	const std::vector<int> expected{4, 3, 2};
	EXPECT_EQ(hs.Scores(), expected);
}

// 排名：比它高的分数数量 + 1；同分保持相同名次
TEST(HighScore, GetRankCountsHigherScores)
{
	TempScoreFile file;
	file.Write("100\n80\n60\n40\n20\n");

	thunder::FileScoreStore store(file.Path());
	thunder::HighScore hs(store);
	hs.Load();

	EXPECT_EQ(hs.GetRank(200), 1);
	EXPECT_EQ(hs.GetRank(100), 1);
	EXPECT_EQ(hs.GetRank(90), 2);
	EXPECT_EQ(hs.GetRank(20), 5);
	EXPECT_EQ(hs.GetRank(10), 6);
}
