// 排行榜规则实现。规则与重构前 HighScore 保持一致（含"只取前 max_keep 条再排序"的现状语义），
// 仅把文件读写替换为注入的 ScoreStore。

#include "game/highscore.h"

#include <algorithm>
#include <functional>

namespace thunder
{

HighScore::HighScore(ScoreStore& store, std::size_t max_keep): store_(store), max_keep_(max_keep) {}

void HighScore::EnsureNotEmpty()
{
	if(scores_.empty())
		scores_.push_back(0);
}

void HighScore::Load()
{
	scores_.clear();

	// 保持重构前语义：只取存储中的前 max_keep 条，再排序（不是全局 Top N）
	const std::vector<int> loaded = store_.Load();
	for(int s: loaded)
	{
		scores_.push_back(s);
		if(scores_.size() >= max_keep_)
			break;
	}

	std::sort(scores_.begin(), scores_.end(), std::greater<int>());

	if(scores_.size() > max_keep_)
		scores_.resize(max_keep_);

	EnsureNotEmpty();
}

void HighScore::Save() const
{
	store_.Save(scores_);
}

void HighScore::Submit(int score)
{
	scores_.push_back(score);

	std::sort(scores_.begin(), scores_.end(), std::greater<int>());

	if(scores_.size() > max_keep_)
		scores_.resize(max_keep_);

	EnsureNotEmpty();
	Save();
}

const std::vector<int>& HighScore::Scores() const
{
	return scores_;
}

int HighScore::GetRank(int score) const
{
	int rank = 1;
	for(int s: scores_)
	{
		if(score < s)
			rank++;
		else
			break;
	}
	return rank;
}

} // namespace thunder
