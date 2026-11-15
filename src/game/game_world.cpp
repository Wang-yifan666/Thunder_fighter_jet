// GameWorld：对局状态与更新逻辑（平台无关）。
// 这里的内容来自重构前的 def.cpp（ResetGame / Level / score / CheckPlayerCollision /
// UpdateBullets）与 DrawFrame 中的逻辑半段，调用顺序与数值保持逐行一致。

#include "game/game_world.h"

#include <algorithm>

using namespace std::chrono;

namespace thunder
{

GameWorld::GameWorld(std::uint32_t rng_seed)
{
	if(rng_seed == 0)
		rng_.seed(std::random_device{}());
	else
		rng_.seed(rng_seed);
}

void GameWorld::Reset(Difficulty difficulty)
{
	difficulty_ = difficulty;
	score_ = 0;
	level_ = 1;

	elapsed_ = steady_clock::duration::zero();
	elapsed_seconds_ = 0;
	last_score_time_ = 0;
	last_level_up_time_ = 0;

	enemies_.clear();
	pending_enemies_.clear();
	bullets_.clear();
	pickups_.clear();
	pickup_move_counter_ = 0;

	switch(difficulty_)
	{
	case Difficulty::Easy:
		life_number = GameConfig::kEasyLife;
		bullets_remaining_ = GameConfig::kEasyBullets;
		break;
	case Difficulty::Normal:
		life_number = GameConfig::kNormalLife;
		bullets_remaining_ = GameConfig::kNormalBullets;
		break;
	case Difficulty::Hard:
		life_number = GameConfig::kHardLife;
		bullets_remaining_ = GameConfig::kHardBullets;
		break;
	}

	player_x_ = GameConfig::kScreenWidth / 2 - 1;
	player_y_ = GameConfig::kScreenHeight - 1;

	enemy_move_counter_ = 0;
	enemy_move_interval_ = GameConfig::kInitialEnemyMoveInterval;
	remaining_rows_in_batch_ = 0;

	is_invincible_ = false;
	cheat_invincible_ = false;
	invincible_remaining_ = steady_clock::duration::zero();

	Make_enermy();
}

void GameWorld::Tick(const GameInput& input, steady_clock::duration dt)
{
	enemy_move_counter_++;

	elapsed_ += dt;
	elapsed_seconds_ = static_cast<int>(duration_cast<seconds>(elapsed_).count());

	// 无敌时间：重构前用绝对时间点 invincible_until_ 判断，这里用剩余时长递减，
	// 语义一致（暂停时不调用 Tick，因此不会流逝）。
	if(is_invincible_)
	{
		invincible_remaining_ -= dt;
		if(invincible_remaining_ <= steady_clock::duration::zero())
		{
			invincible_remaining_ = steady_clock::duration::zero();
			is_invincible_ = false;
		}
	}

	if(enemy_move_counter_ >= enemy_move_interval_)
	{
		MoveEnemies(enemies_, level_, GameConfig::kScreenWidth,
		            GameConfig::kScreenHeight);
		enemy_move_counter_ = 0;
	}

	if(!pending_enemies_.empty())
	{
		SpawnEnemiesFromPending();
	}

	Level(input);
	AccrueScore();

	const int player_w = GameConfig::kPlayerWidth;
	const int player_h = GameConfig::kPlayerHeight;

	if(input.fire && bullets_remaining_ > 0)
	{
		int bx = player_x_ + 1;
		int by = player_y_ - 1;

		if(by < 0)
			by = 0;

		bullets_.emplace_back(bx, by);
		bullets_remaining_--;
	}

	if(input.left)
		player_x_--;
	if(input.right)
		player_x_++;
	if(input.up)
		player_y_--;
	if(input.down)
		player_y_++;

	// 作弊键（与原实现一致：按住即重复触发，不是按下沿触发）
	if(input.digit[8])
		Clear_Current_Enemies();
	if(input.digit[7])
		cheats_kills();
	if(input.digit[6])
		cheats_life();
	if(input.digit[5])
		cheats_invincible();
	if(input.digit[4])
		cheats_addscore();
	if(input.digit[3])
		cheats_addbullters();
	if(input.digit[2])
		kill_self();
	if(input.digit[1])
		cheats_godlike();

	player_x_ = std::clamp(player_x_, 0, GameConfig::kScreenWidth - player_w);
	player_y_ = std::clamp(player_y_, 0, GameConfig::kScreenHeight - player_h);

	// 掉落物先下落再判定拾取，保证用的是本帧位置
	UpdatePickups();
	CheckPickupCollect();

	CheckPlayerCollision();

	UpdateBullets();
}

void GameWorld::Level(const GameInput& input)
{
	if(elapsed_seconds_ - last_level_up_time_ >= GameConfig::kLevelUpTime)
	{
		level_++;
		last_level_up_time_ = elapsed_seconds_;

		bullets_remaining_ += GameConfig::kBulletsPerLevelUp;

		Make_enermy();
	}

	// 作弊：强制升级（按住即连续触发，原实现如此）
	if(input.digit[9])
	{
		level_++;
		Make_enermy();
		bullets_remaining_ += GameConfig::kBulletsPerLevelUp;
	}

	int min_interval = GameConfig::kMinEnemyMoveInterval;
	int max_interval = GameConfig::kMaxEnemyMoveInterval;
	enemy_move_interval_ =
	    max_interval
	    - (level_ - 1) * (max_interval - min_interval)
	          / GameConfig::kLevelMoveIntervalDivisor;
	enemy_move_interval_ = std::max<int>(enemy_move_interval_, min_interval);
}

void GameWorld::AccrueScore()
{
	if(elapsed_seconds_ > last_score_time_)
	{
		score_ += GameConfig::kScorePerSecond;
		last_score_time_ = elapsed_seconds_;
	}
}

WorldSnapshot GameWorld::Snapshot() const
{
	WorldSnapshot snapshot;

	snapshot.difficulty = difficulty_;
	snapshot.player_x = player_x_;
	snapshot.player_y = player_y_;
	snapshot.life = life_number;
	snapshot.invincible = is_invincible_ || cheat_invincible_;

	snapshot.enemies.reserve(enemies_.size());
	for(const auto& e: enemies_)
	{
		if(!e.alive)
			continue;
		snapshot.enemies.push_back(EntityView{e.x, e.y, e.width, e.height});
	}

	snapshot.bullets.reserve(bullets_.size());
	for(const auto& b: bullets_)
		snapshot.bullets.push_back(EntityView{b.x, b.y, 1, 1});

	snapshot.pickups.reserve(pickups_.size());
	for(const auto& p: pickups_)
		snapshot.pickups.push_back(PickupView{
		    p.x, p.y, GameConfig::kPickupSize, GameConfig::kPickupSize,
		    p.kind});

	snapshot.score = score_;
	snapshot.level = level_;
	snapshot.bullets_remaining = bullets_remaining_;
	snapshot.elapsed_seconds = elapsed_seconds_;
	snapshot.pending_enemies = static_cast<int>(pending_enemies_.size());
	snapshot.over = life_number <= 0;

	return snapshot;
}

void GameWorld::CheckPlayerCollision()
{
	if(is_invincible_ || cheat_invincible_)
		return;

	const int player_w = GameConfig::kPlayerWidth;
	const int player_h = GameConfig::kPlayerHeight;

	int px = player_x_;
	int py = player_y_;

	for(auto& e: enemies_)
	{
		if(!e.alive)
			continue;

		int ex = e.x;
		int ey = static_cast<int>(e.y);
		int ew = e.width;
		int eh = e.height;

		bool no_overlap = (px + player_w <= ex)
		    || (ex + ew <= px)
		    || (py + player_h <= ey)
		    || (ey + eh <= py);

		if(no_overlap)
			continue;

		life_number--;
		is_invincible_ = true;
		invincible_remaining_ =
		    std::chrono::seconds(GameConfig::kInvincibleDurationSec);

		if(life_number < 0)
			life_number = 0;

		break;
	}
}

void GameWorld::UpdateBullets()
{
	if(bullets_.empty())
		return;

	for(auto& b: bullets_)
	{
		b.y -= 1;
	}

	bullets_.erase(std::remove_if
	               (bullets_.begin(),
	                bullets_.end(),
	                [](const Bullet& b)
	                {
		                return b.y < 0;
	                }),
	               bullets_.end());

	for(auto b_it = bullets_.begin(); b_it != bullets_.end();)
	{
		bool bullet_consumed = false;

		for(auto e_it = enemies_.begin(); e_it != enemies_.end();)
		{
			Enemy& e = *e_it;
			if(!e.alive)
			{
				e_it = enemies_.erase(e_it);
				continue;
			}

			int ex = e.x;
			int ey = e.y;
			int ew = e.width;
			int eh = e.height;

			bool hit = (b_it->y >= ey)
			    && (b_it->y < ey + eh)
			    && (b_it->x >= ex)
			    && (b_it->x < ex + ew);

			if(hit)
			{
				e_it = enemies_.erase(e_it);
				score_ += GameConfig::kScorePerKill;
				bullet_consumed = true;
				bullets_remaining_ += GameConfig::kBulletsPerKill;
				MaybeDropPickup(ex, ey);
				break;
			}
			else
			{
				e_it++;
			}
		}

		if(bullet_consumed)
		{
			b_it = bullets_.erase(b_it);
		}
		else
		{
			b_it++;
		}
	}
}

void GameWorld::MaybeDropPickup(int enemy_x, int enemy_y)
{
	// 掉落率随难度变化：低难度给新手保护，高难度少给补给
	int health_percent = GameConfig::kNormalHealthDropPercent;
	int ammo_percent = GameConfig::kNormalAmmoDropPercent;
	switch(difficulty_)
	{
	case Difficulty::Easy:
		health_percent = GameConfig::kEasyHealthDropPercent;
		ammo_percent = GameConfig::kEasyAmmoDropPercent;
		break;
	case Difficulty::Normal:
		health_percent = GameConfig::kNormalHealthDropPercent;
		ammo_percent = GameConfig::kNormalAmmoDropPercent;
		break;
	case Difficulty::Hard:
		health_percent = GameConfig::kHardHealthDropPercent;
		ammo_percent = GameConfig::kHardAmmoDropPercent;
		break;
	}

	// 一次掷骰切分两个区间：先血包，再弹药包，其余不掉
	std::uniform_int_distribution<int> roll(1, 100);
	const int value = roll(rng_);

	PickupKind kind;
	if(value <= health_percent)
		kind = PickupKind::Health;
	else if(value <= health_percent + ammo_percent)
		kind = PickupKind::Ammo;
	else
		return;

	const int size = GameConfig::kPickupSize;
	const int enemy_w = GameConfig::kEnemyWidth;

	// 对齐到敌机中心，并保证整块道具在屏幕内
	int x = enemy_x + enemy_w / 2 - size / 2;
	x = std::clamp(x, 0, GameConfig::kScreenWidth - size);

	pickups_.emplace_back(x, enemy_y, kind);
}

void GameWorld::UpdatePickups()
{
	if(pickups_.empty())
	{
		pickup_move_counter_ = 0;
		return;
	}

	// 与敌机一样按固定间隔下落。
	// 注意 y 是格子坐标，逐帧移动会让道具不到一秒就穿过整个棋盘。
	if(++pickup_move_counter_ < GameConfig::kPickupMoveInterval)
		return;

	pickup_move_counter_ = 0;

	for(auto& item: pickups_)
		item.y += GameConfig::kPickupSpeed;

	pickups_.erase(std::remove_if(pickups_.begin(), pickups_.end(),
	                              [](const Pickup& p) {
		                              return p.y
		                                  >= GameConfig::kScreenHeight;
	                              }),
	               pickups_.end());
}

void GameWorld::CheckPickupCollect()
{
	if(pickups_.empty())
		return;

	const int size = GameConfig::kPickupSize;

	// 拾取用"视觉身体"而不是伤害命中框：
	// 精灵底边对齐命中框底边、向上悬垂 kPlayerGrabHeight 行，
	// 玩家看到的是这 4 行，就该按这 4 行判拾取。
	const int grab_w = GameConfig::kPlayerWidth;
	const int grab_h = GameConfig::kPlayerGrabHeight;
	const int grab_x = player_x_;
	const int grab_y =
	    player_y_ + GameConfig::kPlayerHeight - grab_h; // 底边对齐

	for(auto it = pickups_.begin(); it != pickups_.end();)
	{
		const bool no_overlap = (grab_x + grab_w <= it->x)
		    || (it->x + size <= grab_x) || (grab_y + grab_h <= it->y)
		    || (it->y + size <= grab_y);

		if(no_overlap)
		{
			++it;
			continue;
		}

		switch(it->kind)
		{
		case PickupKind::Health:
			life_number += GameConfig::kHealthPickupLife;
			break;
		case PickupKind::Ammo:
			bullets_remaining_ += GameConfig::kAmmoPickupBullets;
			break;
		}

		it = pickups_.erase(it);
	}
}

} // namespace thunder
