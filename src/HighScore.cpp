#include "HighScore.h"

#include <algorithm>
#include <fstream>
#include <functional>

HighScore::HighScore(std::string file_path, std::size_t max_keep):
file_path_(std::move(file_path)), max_keep_(max_keep)
{
}

void HighScore::EnsureNotEmpty()
{
	if(scores_.empty())
		scores_.push_back(0);
}

void HighScore::Load()
{
	scores_.clear();

	std::ifstream fin(file_path_);
	if(!fin)
	{
		//文件不存在：创建一个新文件，写入0
		std::ofstream fout(file_path_);
		if(fout)
			fout << 0 << "\n";

		scores_.push_back(0);
		return;
	}

	int s = 0;
	while(fin >> s)
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
	std::ofstream fout(file_path_);
	if(!fout)
		return;

	for(int s: scores_)
		fout << s << "\n";
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
