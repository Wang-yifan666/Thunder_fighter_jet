// GameWorld 的敌机部分：刷新、批次生成、移动、重叠判定。
// 内容来自重构前的 enemy.cpp，仅把类名改为 GameWorld 并移入 thunder 命名空间。

#include "game/game_world.h"
#include "game/game_config.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <random>
#include <utility>

namespace
{

/// 每批刷怪上限（随难度变化）。
/// 原实现用命名空间级可变量保存，属于跨调用/跨局共享的隐藏状态；
/// 改为纯函数查表后，任何一局只依赖入参。
struct BatchLimits
{
	int max_batch_count;
	int max_per_batch;
};

BatchLimits BatchLimitsFor(Difficulty difficulty)
{
	switch(difficulty)
	{
	case Difficulty::Easy:
		return {GameConfig::kEasyMaxBatchCount, GameConfig::kEasyMaxPerBatch};
	case Difficulty::Normal:
		return {GameConfig::kNormalMaxBatchCount, GameConfig::kNormalMaxPerBatch};
	case Difficulty::Hard:
		return {GameConfig::kHardMaxBatchCount, GameConfig::kHardMaxPerBatch};
	}
	return {GameConfig::kNormalMaxBatchCount, GameConfig::kNormalMaxPerBatch};
}

/// 精英出现的概率上限（百分比），随难度提高。
int EliteCapPercent(Difficulty difficulty)
{
	switch(difficulty)
	{
	case Difficulty::Easy:
		return GameConfig::kEliteCapEasyPercent;
	case Difficulty::Normal:
		return GameConfig::kEliteCapNormalPercent;
	case Difficulty::Hard:
		return GameConfig::kEliteCapHardPercent;
	}
	return GameConfig::kEliteCapNormalPercent;
}

/// 普通小怪自带护盾的概率（百分比），随难度提高。
int ShieldedEnemyPercent(Difficulty difficulty)
{
	switch(difficulty)
	{
	case Difficulty::Easy:
		return GameConfig::kEasyShieldedEnemyPercent;
	case Difficulty::Normal:
		return GameConfig::kNormalShieldedEnemyPercent;
	case Difficulty::Hard:
		return GameConfig::kHardShieldedEnemyPercent;
	}
	return GameConfig::kNormalShieldedEnemyPercent;
}

} //namespace

namespace thunder
{

int GameWorld::EliteSpawnPercent() const
{
	if(level_ <= GameConfig::kEliteMinLevel)
		return 0;

	// 指数饱和：p(L) = cap * (1 - exp(-(L - L0) / K))
	// 前 10 级慢慢出现，到 40 级左右过半，100 级前后基本顶到 cap。
	const double t = static_cast<double>(level_ - GameConfig::kEliteMinLevel)
	    / static_cast<double>(GameConfig::kEliteSpawnCurveK);
	const double p = static_cast<double>(EliteCapPercent(difficulty_)) * (1.0 - std::exp(-t));
	return static_cast<int>(std::lround(p));
}

int GameWorld::CrowdBatchBonus(int backlog)
{
	if(backlog <= GameConfig::kCrowdBatchThreshold)
		return 0;

	// 超过阈值后每多 kCrowdBatchPerStep 只加 1，最多加 kCrowdBatchMaxBonus 只
	const int extra =
	    1 + (backlog - GameConfig::kCrowdBatchThreshold) / GameConfig::kCrowdBatchPerStep;
	return std::min(extra, GameConfig::kCrowdBatchMaxBonus);
}

void GameWorld::SpawnEnemyWave()
{
	std::uniform_int_distribution<int> randSpeed(GameConfig::kMinEnemySpeed,
	                                             GameConfig::kMaxEnemySpeed);
	std::uniform_int_distribution<int> randPercent(1, 100);

	int base = level_ * 2;
	int total_to_spawn = base; // Normal 档默认值（switch 已覆盖全部枚举）
	switch(difficulty_)
	{
	case Difficulty::Easy:
		total_to_spawn = std::max(2, base - 2);
		break;
	case Difficulty::Normal:
		total_to_spawn = base;
		break;
	case Difficulty::Hard:
		total_to_spawn = base * 2;
		break;
	}

	const int elite_percent = EliteSpawnPercent();
	// 5 级之后普通小怪也会随机带盾：先破盾、再补一发，共两下
	const bool shields_enabled = (level_ > GameConfig::kEnemyShieldMinLevel);
	const int shield_percent = ShieldedEnemyPercent(difficulty_);

	for(int i = 0; i < total_to_spawn; ++i)
	{
		const int speed = randSpeed(rng_);

		// 等级够了以后按概率出精英；精英自身再按概率带盾（带盾 = 要多打一下）。
		if(elite_percent > 0 && randPercent(rng_) <= elite_percent)
		{
			const bool has_shield = randPercent(rng_) <= GameConfig::kEliteShieldPercent;
			pending_enemies_.push_back(Enemy::Elite(0, 0, speed, has_shield));
		}
		else
		{
			const bool has_shield = shields_enabled && randPercent(rng_) <= shield_percent;
			pending_enemies_.emplace_back(0, 0, speed, GameConfig::kEnemyWidth,
			                              GameConfig::kEnemyHeight, EnemyKind::Normal,
			                              has_shield);
		}
	}

	//开启一波新的刷新
	remaining_rows_in_batch_ = 0;
}

void GameWorld::ClearActiveEnemies()
{
	enemies_.clear();
}

void GameWorld::SpawnEnemiesFromPending()
{
	if(pending_enemies_.empty())
		return;

	const BatchLimits limits = BatchLimitsFor(difficulty_);

	//如果当前这一波还没决定“总共有多少批”，在这里随机一个 1~15 批
	if(remaining_rows_in_batch_ == 0)
	{
		//这波最多能有多少批？理论上<=pending 数量，外加上限 15
		int max_possible_batches =
		    std::min<int>(limits.max_batch_count, static_cast<int>(pending_enemies_.size()));

		if(max_possible_batches <= 0)
			return;

		std::uniform_int_distribution<int> batch_dist(1, max_possible_batches);
		remaining_rows_in_batch_ = batch_dist(rng_);
	}

	// 新一批敌机生成在 y = 0；精英高 3 行，所以要求这 3 行都空着。
	// 曾经这里写死成 e.y < 1（只清 1 行）：命中框加高到 2 行之后，
	// y == 1 的旧敌机正好和新敌机压在第 1 行上，视觉上叠在一起。
	bool top_row_clear = std::none_of(enemies_.begin(), enemies_.end(), [](const Enemy& e) {
		return e.y < GameConfig::kEliteHeight;
	});

	if(!top_row_clear)
		return; // 顶部还没空出来，等下一帧再说

	// 生成当前这一批：个数随机，全部放在 y = 0，x 随机且尽量不重叠
	const int max_x_pad = GameConfig::kEliteWidth; // 两架之间至少留出精英那么宽

	int remaining = static_cast<int>(pending_enemies_.size());
	if(remaining <= 0)
		return;

	// 后期堆怪时的泄压阀：场上 + 排队的超过阈值就每批多放几只，
	// 免得全挤在看不见的排队里、场上反而越来越稀。
	const int backlog = static_cast<int>(enemies_.size()) + remaining;
	const int crowd_bonus = CrowdBatchBonus(backlog);

	int max_in_row = std::min<int>(limits.max_per_batch + crowd_bonus, remaining);
	std::uniform_int_distribution<int> count_dist(1, max_in_row);
	int enemies_in_row = count_dist(rng_);

	std::vector<int> used_x;
	std::vector<Enemy> spawned_row;

	for(int i = 0; i < enemies_in_row && !pending_enemies_.empty(); ++i)
	{
		Enemy e = std::move(pending_enemies_.front());
		pending_enemies_.pop_front();

		const int max_x = GameConfig::kScreenWidth - e.width;

		int x = 0;
		int attempts = 0;
		do
		{
			std::uniform_int_distribution<int> x_dist(0, max_x);
			x = x_dist(rng_);
			attempts++;
		} while(std::any_of(used_x.begin(), used_x.end(),
		                    [&](int ux) {
			                    return std::abs(ux - x) < max_x_pad;
		                    })
		        && attempts < 100);

		used_x.push_back(x);

		e.x = x;
		e.y = 0; //顶行生成
		// 上一帧坐标一并重置：排队回来的敌机还留着上次在场上的落点，
		// 不清干净会让扫掠判定的区间算错
		e.prev_x = x;
		e.prev_y = 0;
		spawned_row.push_back(e);
	}

	enemies_.insert(enemies_.end(), spawned_row.begin(), spawned_row.end());

	//当前这批刷完了，这一波的剩余批次数量减一
	remaining_rows_in_batch_--;

	//如果这一波批次刷完了，但 pending 里还有敌人，下一次再进来会重新随机新一波批数
	if(remaining_rows_in_batch_ < 0)
		remaining_rows_in_batch_ = 0;
}

void GameWorld::MoveEnemies(std::vector<Enemy>& active_enemies, int level, int screen_width,
                            int screen_height)
{
	std::vector<Enemy> to_pending; // 到底部的敌人

	// 两架敌机在格子意义上的矩形是否相交。
	auto overlaps = [](const Enemy& a, int ax, int ay, const Enemy& b) {
		return !((ax + a.width <= b.x) || (b.x + b.width <= ax) || (ay + a.height <= b.y)
		         || (b.y + b.height <= ay));
	};

	// 指定敌机移到 (x, y) 后会不会撞上别的敌机。
	auto would_collide = [&](std::size_t index, int x, int y) {
		const Enemy& me = active_enemies[index];
		for(std::size_t i = 0; i < active_enemies.size(); ++i)
		{
			if(i == index)
				continue;
			if(overlaps(me, x, y, active_enemies[i]))
				return true;
		}
		return false;
	};

	for(std::size_t i = 0; i < active_enemies.size();)
	{
		Enemy& e = active_enemies[i];

		// 向下移动
		const int target_y = e.y + e.speed;

		if(target_y >= screen_height) // 到底部
		{
			// 放回 pending
			to_pending.push_back(e);
			active_enemies.erase(active_enemies.begin() + static_cast<std::ptrdiff_t>(i));
			continue; // erase 之后同一位置就是下一个元素，不能再 ++i
		}

		// 水平移动
		int target_x = e.x;
		if(level >= GameConfig::kStartToMoveLeftRight)
		{
			std::uniform_int_distribution<int> dir_dist(-1, 1);
			const int dir = dir_dist(rng_); // -1 左, 0 不动, 1 右
			target_x = e.x + dir;
			if(target_x < 0)
				target_x = 0;
			if(target_x + e.width >= screen_width)
				target_x = screen_width - e.width;
		}

		// 能走就走整步；被别的敌机挡住时退化为只下移 / 只横移，绝不重叠。
		// 命中框是 3x2 格：同一列相差 1 行就会在视觉上叠在一起，
		// 所以"速度快的追尾速度慢的"必须被挡住，不能穿过去。
		if(!would_collide(i, target_x, target_y))
		{
			e.x = target_x;
			e.y = target_y;
		}
		else if(!would_collide(i, e.x, target_y))
		{
			e.y = target_y;
		}
		else if(!would_collide(i, target_x, e.y))
		{
			e.x = target_x;
		}

		++i;
	}

	// 放回 pending
	pending_enemies_.insert(pending_enemies_.end(), to_pending.begin(), to_pending.end());
}

} // namespace thunder
