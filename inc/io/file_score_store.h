#pragma once

#include <string>

#include "game/score_store.h"

namespace thunder
{

/// 基于文件的分数存储（唯一涉及文件 IO 的实现）。
/// 文件格式与默认路径保持重构前一致：每行一个分数。
class FileScoreStore : public ScoreStore
{
public:
	explicit FileScoreStore(std::string path = "../highscore.txt");

	std::vector<int> Load() override;
	void Save(const std::vector<int>& scores) override;

private:
	std::string path_;
};

} // namespace thunder
