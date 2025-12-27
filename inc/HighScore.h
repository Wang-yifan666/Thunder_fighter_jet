#pragma once
#include <string>
#include <vector>

class HighScore
{
public:
	explicit HighScore(
	    std::string file_path = "../highscore.txt",
	    std::size_t max_keep = 5);

	//读取文件到内存（如果文件不存在会创建并写入 0）
	void Load();

	//保存内存到文件
	void Save() const;

	//提交一个分数（自动排序、截断 topN，并保存）
	void Submit(int score);

	//获取 Top 列表（保证至少有一个元素）
	const std::vector<int>& Scores() const;

private:
	void EnsureNotEmpty();

private:
	std::string file_path_;
	std::size_t max_keep_;
	std::vector<int> scores_;
};
