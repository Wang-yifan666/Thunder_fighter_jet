// 文件版 ScoreStore。行为与重构前的 HighScore 文件读写保持一致：
// - 文件不存在时创建并写入 0
// - 每行一个分数；Save 覆盖写入
// - Load 返回文件中的全部整数（截断规则由 HighScore 负责）

#include "io/file_score_store.h"

#include <fstream>
#include <utility>

namespace thunder
{

FileScoreStore::FileScoreStore(std::string path): path_(std::move(path)) {}

std::vector<int> FileScoreStore::Load()
{
	std::vector<int> scores;

	std::ifstream fin(path_);
	if(!fin)
	{
		// 文件不存在：创建并写入 0（与原 HighScore::Load 行为一致）
		std::ofstream fout(path_);
		if(fout)
			fout << 0 << "\n";

		scores.push_back(0);
		return scores;
	}

	int s = 0;
	while(fin >> s)
		scores.push_back(s);

	return scores;
}

void FileScoreStore::Save(const std::vector<int>& scores)
{
	std::ofstream fout(path_);
	if(!fout)
		return;

	for(int s: scores)
		fout << s << "\n";
}

} // namespace thunder
