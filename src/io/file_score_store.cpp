// 文件版 ScoreStore。
// - 文件不存在时创建并写入 0
// - 每行一个分数；Save 覆盖写入
// - Load 返回文件中的全部整数（截断规则由 HighScore 负责）
//
// 写入一律用二进制模式 + 显式 '\n'：
// Windows 下文本模式会把 '\n' 变成 CRLF，导致同一个存档文件在 git 里
// 因为"只有行尾变了"反复变脏。（历史上仓库根目录的 highscore.txt 就是这样。）

#include "io/file_score_store.h"

#include <fstream>
#include <utility>

namespace thunder
{

FileScoreStore::FileScoreStore(std::string path): path_(std::move(path)) {}

std::vector<int> FileScoreStore::Load()
{
	std::vector<int> scores;

	std::ifstream fin(path_, std::ios::binary);
	if(!fin)
	{
		// 文件不存在：创建并写入 0
		std::ofstream fout(path_, std::ios::binary | std::ios::trunc);
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
	std::ofstream fout(path_, std::ios::binary | std::ios::trunc);
	if(!fout)
		return;

	for(int s: scores)
		fout << s << "\n";
}

} // namespace thunder
