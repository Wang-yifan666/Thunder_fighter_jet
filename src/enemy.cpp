#include "def.h"

#include <algorithm>
#include <random>

namespace
{
constexpr int kScreenWidth = 98;
constexpr int kScreenHeight = 40;
constexpr int kStartToMoveDown = 2;
constexpr int kStartToMoveLeftRight = 4;
constexpr int kMaxBatchCount = 15; //一波最多15批
constexpr int kMaxPerBatch = 3;    //每批最多3个敌人

std::mt19937& Rng()
{ //全局随机数生成器
	static std::mt19937 rng{std::random_device{}()};
	return rng;
}
} //namespace

void ThunderFighter::Make_enermy()
{
	std::uniform_int_distribution<int> randSpeed(1, 3);

	int total_to_spawn = level_ * 2;

	for(int i = 0; i < total_to_spawn; ++i)
		pending_enemies_.emplace_back(0, 0, randSpeed(Rng()));

	//开启一波新的刷怪，让SpawnEnemiesFromPending随机决定这波有几批
	remaining_rows_in_batch_ = 0;
}

void ThunderFighter::Clear_Current_Enemies()
{
	enemies_.clear();
	alive_enemies_count = 0;
}

void ThunderFighter::cheats_kills()
{
	enemies_.clear();
	pending_enemies_.clear();
	alive_enemies_count = 0;
}

void ThunderFighter::SpawnEnemiesFromPending()
{
	if(pending_enemies_.empty())
		return;

	// 1. 如果当前这一波还没决定“总共有多少批”，在这里随机一个 1~15 批
	if(remaining_rows_in_batch_ == 0)
	{
		// 这波最多能有多少批？理论上 <= pending 数量，外加上限 15
		int max_possible_batches = std::min(
		    kMaxBatchCount, static_cast<int>(pending_enemies_.size()));

		if(max_possible_batches <= 0)
			return;

		std::uniform_int_distribution<int> batch_dist(
		    1, max_possible_batches);
		remaining_rows_in_batch_ = batch_dist(Rng());
	}

	// 2. 只有当前“顶行完全空”的时候，才生成下一批
	bool top_row_clear = std::none_of(
	    enemies_.begin(), enemies_.end(), [](const Enemy& e) {
		    return e.y < 1; // y == 0 认为在顶行，<1 即顶行及以上
	    });

	if(!top_row_clear)
		return; // 顶行还没空，等下一帧再说

	// 3. 生成当前这一批：1~3 个敌人，全部放在 y = 0，x 随机且尽量不重叠
	const int enemy_width = 3;
	const int max_x = kScreenWidth - enemy_width;

	int remaining = static_cast<int>(pending_enemies_.size());
	if(remaining <= 0)
		return;

	int max_in_row = std::min(kMaxPerBatch, remaining);
	std::uniform_int_distribution<int> count_dist(1, max_in_row);
	int enemies_in_row = count_dist(Rng());

	std::vector<int> used_x;
	std::vector<Enemy> spawned_row;

	for(int i = 0; i < enemies_in_row; ++i)
	{
		int x = 0;
		int attempts = 0;
		do
		{
			std::uniform_int_distribution<int> x_dist(0, max_x);
			x = x_dist(Rng());
			attempts++;
		} while(std::any_of(used_x.begin(), used_x.end(),
		                    [&](int ux) {
			                    return std::abs(ux - x) < enemy_width;
		                    })
		        && attempts < 100);

		used_x.push_back(x);

		Enemy e = pending_enemies_.front();
		pending_enemies_.erase(pending_enemies_.begin());

		e.x = x;
		e.y = 0; // 顶行生成
		spawned_row.push_back(e);
	}

	enemies_.insert(enemies_.end(), spawned_row.begin(),
	                spawned_row.end());

	// 4. 当前这批刷完了，这一波的剩余批次数量减一
	remaining_rows_in_batch_--;

	// 如果这一波批次刷完了，但 pending 里还有敌人，下一次再进来会重新随机新一波批数
	if(remaining_rows_in_batch_ < 0)
		remaining_rows_in_batch_ = 0;
}

bool ThunderFighter::IsOverlapping(const Enemy& a, const Enemy& b)
{
	return !(a.x + a.width <= b.x     //
	         || b.x + b.width <= a.x  //
	         || a.y + a.height <= b.y //
	         || b.y + b.height <= a.y);
}

bool ThunderFighter::WillOverlap(const Enemy& cur,
                                 const std::vector<Enemy>& enemies,
                                 int new_x, int new_y)
{
	Enemy temp = cur;
	temp.x = new_x;
	temp.y = new_y;
	for(auto& e: enemies)
	{
		if(&e == &cur)
			continue;
		if(IsOverlapping(temp, e))
			return true;
	}
	return false;
}

void ThunderFighter::MoveEnemies(std::vector<Enemy>& active_enemies,
                                 int level, int screen_width,
                                 int screen_height)
{
	std::vector<Enemy> to_pending; // 到底部的敌人

	for(auto it = active_enemies.begin(); it != active_enemies.end();)
	{
		// 向下移动
		it->y += it->speed;

		if(it->y >= screen_height) // 到底部
		{
			// 放回 pending
			to_pending.push_back(*it);
			it = active_enemies.erase(it);
		}
		else
		{
			// 水平移动
			if(level >= kStartToMoveLeftRight)
			{
				int dir = rand() % 3 - 1; // -1 左, 0 不动, 1 右
				int new_x = it->x + dir;
				if(new_x < 0)
					new_x = 0;
				if(new_x + it->width >= screen_width)
					new_x = screen_width - it->width;
				it->x = new_x;
			}
			++it;
		}
	}

	// 放回 pending
	pending_enemies_.insert(pending_enemies_.end(), to_pending.begin(),
	                        to_pending.end());
}
