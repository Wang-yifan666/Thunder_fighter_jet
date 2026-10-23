#pragma once

#include <cstddef>
#include <vector>

#include "game/score_store.h"

namespace thunder
{

/// 排行榜规则：排序、截断到 Top N、名次计算。
/// 不直接接触文件，存取通过 ScoreStore 注入。
class HighScore
{
public:
	explicit HighScore(ScoreStore& store, std::size_t max_keep = 5);

	//读取存储到内存（空数据时保证至少有一个 0）
	void Load();

	//保存内存到存储
	void Save() const;

	//提交一个分数（自动排序、截断 topN，并保存）
	void Submit(int score);

	//获取 Top 列表（保证至少有一个元素）
	const std::vector<int>& Scores() const;

	//计算给定分数的排名（1-based，越高分排名越靠前）
	int GetRank(int score) const;

private:
	void EnsureNotEmpty();

private:
	ScoreStore& store_;
	std::size_t max_keep_;
	std::vector<int> scores_;
};

} // namespace thunder
