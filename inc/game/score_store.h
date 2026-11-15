#pragma once

#include <vector>

namespace thunder
{

/// 分数持久化接口（纯逻辑：不接触文件系统）。
/// 具体实现由 thunder_io 提供（FileScoreStore），测试用 MemoryScoreStore。
class ScoreStore
{
public:
	virtual ~ScoreStore() = default;

	/// 返回已保存的原始分数（顺序即存储顺序，可能为空）。
	virtual std::vector<int> Load() = 0;

	virtual void Save(const std::vector<int>& scores) = 0;
};

class MemoryScoreStore: public ScoreStore
{
public:
	std::vector<int> Load() override
	{
		return scores_;
	}

	void Save(const std::vector<int>& scores) override
	{
		scores_ = scores;
	}

private:
	std::vector<int> scores_;
};

} // namespace thunder
