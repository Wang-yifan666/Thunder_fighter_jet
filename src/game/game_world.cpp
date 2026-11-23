// GameWorld：对局状态与更新逻辑（平台无关）。
// 这里的内容来自重构前的 def.cpp（ResetGame / Level / score / CheckPlayerCollision /
// UpdateBullets）与 DrawFrame 中的逻辑半段，调用顺序与数值保持逐行一致。

#include "game/game_world.h"

#include <algorithm>
#include <cmath>

using namespace std::chrono;

namespace
{

/// 四种道具的掉落率（百分比，顺序即掷骰区间顺序）。
struct DropRates
{
	int health;
	int ammo;
	int shield;
	int firepower;
};

DropRates DropRatesFor(Difficulty difficulty)
{
	switch(difficulty)
	{
	case Difficulty::Easy:
		return {GameConfig::kEasyHealthDropPercent, GameConfig::kEasyAmmoDropPercent,
		        GameConfig::kEasyShieldDropPercent, GameConfig::kEasyFirepowerDropPercent};
	case Difficulty::Normal:
		return {GameConfig::kNormalHealthDropPercent, GameConfig::kNormalAmmoDropPercent,
		        GameConfig::kNormalShieldDropPercent, GameConfig::kNormalFirepowerDropPercent};
	case Difficulty::Hard:
		return {GameConfig::kHardHealthDropPercent, GameConfig::kHardAmmoDropPercent,
		        GameConfig::kHardShieldDropPercent, GameConfig::kHardFirepowerDropPercent};
	}
	return {GameConfig::kNormalHealthDropPercent, GameConfig::kNormalAmmoDropPercent,
	        GameConfig::kNormalShieldDropPercent, GameConfig::kNormalFirepowerDropPercent};
}

} // namespace

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

	effects_.clear();
	fire_cooldown_frames_ = 0;
	firepower_frames_left_ = 0;
	player_shield_ = false;

	switch(difficulty_)
	{
	case Difficulty::Easy:
		life_ = GameConfig::kEasyLife;
		bullets_remaining_ = GameConfig::kEasyBullets;
		break;
	case Difficulty::Normal:
		life_ = GameConfig::kNormalLife;
		bullets_remaining_ = GameConfig::kNormalBullets;
		break;
	case Difficulty::Hard:
		life_ = GameConfig::kHardLife;
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

	SpawnEnemyWave();
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

	// 记录移动前的坐标：子弹一帧上移 1 格、敌机一次可能下移 3 格，
	// 只比末位置会漏掉"两边同时动、中途擦身而过"，命中判定要用这两个值做扫掠。
	for(auto& enemy: enemies_)
	{
		enemy.prev_x = enemy.x;
		enemy.prev_y = enemy.y;
	}

	if(enemy_move_counter_ >= enemy_move_interval_)
	{
		MoveEnemies(enemies_, level_, GameConfig::kScreenWidth, GameConfig::kScreenHeight);
		enemy_move_counter_ = 0;
	}

	if(!pending_enemies_.empty())
	{
		SpawnEnemiesFromPending();
	}

	UpdateLevel(input);
	AccrueScore();

	const int player_w = GameConfig::kPlayerWidth;
	const int player_h = GameConfig::kPlayerHeight;

	// 开火节奏：先递减冷却再判断，kFireIntervalFrames = 2 就是"每 2 帧一发"。
	if(fire_cooldown_frames_ > 0)
		--fire_cooldown_frames_;
	if(firepower_frames_left_ > 0)
		--firepower_frames_left_;

	if(input.fire && bullets_remaining_ > 0 && fire_cooldown_frames_ == 0)
	{
		FirePlayerBullets();
		bullets_remaining_--;
		fire_cooldown_frames_ = GameConfig::kFireIntervalFrames;
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
		ClearActiveEnemies();
	if(input.digit[7])
		CheatKillAll();
	if(input.digit[6])
		CheatAddLife();
	if(input.digit[5])
		CheatToggleInvincible();
	if(input.digit[4])
		CheatAddScore();
	if(input.digit[3])
		CheatAddAmmo();
	if(input.digit[2])
		CheatSelfDestruct();
	if(input.digit[1])
		CheatGodMode();

	player_x_ = std::clamp(player_x_, 0, GameConfig::kScreenWidth - player_w);
	player_y_ = std::clamp(player_y_, 0, GameConfig::kScreenHeight - player_h);

	// 掉落物先下落再判定拾取，保证用的是本帧位置
	UpdatePickups();
	CheckPickupCollect();

	CheckPlayerCollision();

	UpdateBullets();

	// 统一清理被标记死亡的敌机：子弹、冲击波都只负责标记，容器操作集中在这里。
	// （不能只放在 UpdateBullets 里——没有子弹时会提前返回，冲击波的击杀就漏掉了。）
	enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
	                              [](const Enemy& e) {
		                              return !e.alive;
	                              }),
	               enemies_.end());

	// 破盾后的无敌帧逐帧递减（不能放进 MoveEnemies：它按移动间隔才跑一次）
	for(auto& enemy: enemies_)
	{
		if(enemy.invuln_frames > 0)
			--enemy.invuln_frames;
	}

	// 特效只影响画面，放在最后推进，避免和判定抢顺序
	UpdateEffects();
}

void GameWorld::UpdateLevel(const GameInput& input)
{
	// 1 级只等 5 秒，其余每级 10 秒
	const int interval =
	    (level_ <= 1) ? GameConfig::kFirstLevelUpTime : GameConfig::kLevelUpTime;

	if(elapsed_seconds_ - last_level_up_time_ >= interval)
	{
		level_++;
		last_level_up_time_ = elapsed_seconds_;

		bullets_remaining_ += GameConfig::kBulletsPerLevelUp;

		SpawnEnemyWave();
	}

	// 作弊：强制升级（按住即连续触发，原实现如此）
	if(input.digit[9])
	{
		level_++;
		SpawnEnemyWave();
		bullets_remaining_ += GameConfig::kBulletsPerLevelUp;
	}

	int min_interval = GameConfig::kMinEnemyMoveInterval;
	int max_interval = GameConfig::kMaxEnemyMoveInterval;
	enemy_move_interval_ = max_interval
	    - (level_ - 1) * (max_interval - min_interval) / GameConfig::kLevelMoveIntervalDivisor;
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
	snapshot.life = life_;
	snapshot.invincible = is_invincible_ || cheat_invincible_;
	snapshot.shielded = player_shield_;
	snapshot.firepower_seconds_left =
	    firepower_frames_left_ * GameConfig::kFrameDelayMs / 1000;

	snapshot.enemies.reserve(enemies_.size());
	for(const auto& e: enemies_)
	{
		if(!e.alive)
			continue;
		snapshot.enemies.push_back(
		    EnemyView{e.x, e.y, e.width, e.height, e.kind, e.has_shield, e.invuln_frames > 0});
	}

	snapshot.bullets.reserve(bullets_.size());
	for(const auto& b: bullets_)
		snapshot.bullets.push_back(EntityView{b.x, b.y, 1, 1});

	snapshot.pickups.reserve(pickups_.size());
	for(const auto& p: pickups_)
		snapshot.pickups.push_back(
		    PickupView{p.x, p.y, GameConfig::kPickupSize, GameConfig::kPickupSize, p.kind});

	snapshot.effects.reserve(effects_.size());
	for(const auto& fx: effects_)
		snapshot.effects.push_back(EffectView{fx.x, fx.y, fx.kind, fx.age, fx.life, fx.radius});

	snapshot.score = score_;
	snapshot.level = level_;
	snapshot.bullets_remaining = bullets_remaining_;
	snapshot.elapsed_seconds = elapsed_seconds_;
	snapshot.pending_enemies = static_cast<int>(pending_enemies_.size());
	snapshot.over = life_ <= 0;

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

		bool no_overlap =
		    (px + player_w <= ex) || (ex + ew <= px) || (py + player_h <= ey) || (ey + eh <= py);

		if(no_overlap)
			continue;

		// 有护盾：先破盾——不掉血、放一次冲击波清场，然后照样进入无敌时间。
		// 无敌用和受伤相同的 1 秒：只给 0.32 秒的话，刚破盾就会被旁边的怪直接撞掉血。
		if(player_shield_)
		{
			player_shield_ = false;
			AddEffect(EffectKind::Shockwave, px + player_w / 2, py,
			          GameConfig::kShockwaveEffectFrames);
			ApplyShockwave();
		}
		else
		{
			life_--;
			if(life_ < 0)
				life_ = 0;
		}

		is_invincible_ = true;
		invincible_remaining_ = std::chrono::seconds(GameConfig::kInvincibleDurationSec);

		break;
	}
}

void GameWorld::ApplyShockwave()
{
	const float cx = static_cast<float>(player_x_) + static_cast<float>(GameConfig::kPlayerWidth) * 0.5f;
	const float cy = static_cast<float>(player_y_) + static_cast<float>(GameConfig::kPlayerHeight) * 0.5f;
	const float radius = GameConfig::kShockwaveRadiusCells;
	const float radius_sq = radius * radius;

	// 先收集目标再结算：ApplyHit 可能把敌机标记为死亡，
	// 直接在遍历里改容器会出问题。
	for(auto& enemy: enemies_)
	{
		if(!enemy.alive)
			continue;

		const float ex = static_cast<float>(enemy.x) + static_cast<float>(enemy.width) * 0.5f;
		const float ey = static_cast<float>(enemy.y) + static_cast<float>(enemy.height) * 0.5f;
		const float dx = ex - cx;
		const float dy = ey - cy;
		if(dx * dx + dy * dy > radius_sq)
			continue;

		ApplyHit(enemy, enemy.x + enemy.width / 2, enemy.y + enemy.height / 2);
	}
}

GameWorld::HitResult GameWorld::ApplyHit(Enemy& enemy, int hit_x, int hit_y)
{
	// 破盾后的无敌帧：这一下完全不算（子弹会穿过去，冲击波也打不动）
	if(enemy.invuln_frames > 0)
		return HitResult::Blocked;

	if(enemy.has_shield)
	{
		enemy.has_shield = false;
		enemy.invuln_frames = GameConfig::kEnemyShieldBreakInvulnFrames;
		AddEffect(EffectKind::Spark, hit_x, hit_y, GameConfig::kShieldBreakSparkFrames);
		return HitResult::ShieldBroken;
	}

	enemy.hp--;
	if(enemy.hp > 0)
	{
		AddEffect(EffectKind::Spark, hit_x, hit_y, GameConfig::kShieldBreakSparkFrames);
		return HitResult::Damaged;
	}

	enemy.alive = false;

	const bool elite = (enemy.kind == EnemyKind::Elite);
	score_ += elite ? GameConfig::kScorePerEliteKill : GameConfig::kScorePerKill;
	bullets_remaining_ += elite ? GameConfig::kBulletsPerEliteKill : GameConfig::kBulletsPerKill;
	MaybeDropPickup(enemy.x, enemy.y);

	return HitResult::Destroyed;
}

void GameWorld::UpdateEffects()
{
	if(effects_.empty())
		return;

	for(auto& fx: effects_)
		++fx.age;

	effects_.erase(std::remove_if(effects_.begin(), effects_.end(),
	                              [](const Effect& fx) {
		                              return fx.age >= fx.life;
	                              }),
	               effects_.end());
}

void GameWorld::AddEffect(EffectKind kind, int cell_x, int cell_y, int life)
{
	// 特效纯视觉，不参与判定；数量天然有限（每帧最多几发子弹、破盾一次一个）
	effects_.emplace_back(cell_x, cell_y, kind, life,
	                      kind == EffectKind::Shockwave ? GameConfig::kShockwaveRadiusCells : 0.0f);
}

void GameWorld::UpdateBullets()
{
	if(bullets_.empty())
		return;

	for(auto& b: bullets_)
	{
		b.y -= 1;
	}

	bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
	                              [](const Bullet& b) {
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

			// 子弹本帧从 y+1 上移到 y；敌机本帧可能从 prev_y 下移 speed（最多 3）格。
			// 只比末位置会漏掉"两边同时移动、中途穿过对方"的情况，这里用扫掠区间求交：
			//   行：子弹扫过 [y, y+1]，敌机扫过 [prev_y, y + height - 1]
			//   列：子弹是 1 格，敌机扫过 [min(prev_x, x), max(prev_x, x) + width - 1]
			// 判定偏保守（可能多算一次"擦边"），方向是对玩家有利。
			const int bullet_top = b_it->y;
			const int bullet_bottom = b_it->y + 1;
			const int enemy_top = e.prev_y;
			const int enemy_bottom = e.y + e.height - 1;
			const int enemy_left = std::min(e.prev_x, e.x);
			const int enemy_right = std::max(e.prev_x, e.x) + e.width - 1;

			const bool hit = (bullet_top <= enemy_bottom) && (bullet_bottom >= enemy_top)
			    && (b_it->x >= enemy_left) && (b_it->x <= enemy_right);

			if(!hit)
			{
				e_it++;
				continue;
			}

			// 破盾无敌期内子弹直接穿过去（不消耗），玩家看得见子弹继续飞；
			// 敌人自身在闪烁，配上火花，不会像"打中了却不算"。
			if(ApplyHit(e, b_it->x, e.y + e.height - 1) == HitResult::Blocked)
			{
				e_it++;
				continue;
			}

			bullet_consumed = true;
			break;
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

void GameWorld::FirePlayerBullets()
{
	const int bx = player_x_ + 1;
	const int by = std::max(0, player_y_ - 1);

	bullets_.emplace_back(bx, by);

	// 火力强化：左右各加一路。贴在屏幕边上时那一侧自然没有子弹（格子坐标越界）。
	if(firepower_frames_left_ > 0)
	{
		if(bx - 1 >= 0)
			bullets_.emplace_back(bx - 1, by);
		if(bx + 1 < GameConfig::kScreenWidth)
			bullets_.emplace_back(bx + 1, by);
	}
}

void GameWorld::MaybeDropPickup(int enemy_x, int enemy_y)
{
	// 掉落率随难度变化：低难度给新手保护，高难度少给补给
	const DropRates rates = DropRatesFor(difficulty_);

	// 一次掷骰切四个区间：先血包、再弹药、护盾、火力，其余不掉
	std::uniform_int_distribution<int> roll(1, 100);
	const int value = roll(rng_);

	PickupKind kind;
	if(value <= rates.health)
		kind = PickupKind::Health;
	else if(value <= rates.health + rates.ammo)
		kind = PickupKind::Ammo;
	else if(value <= rates.health + rates.ammo + rates.shield)
		kind = PickupKind::Shield;
	else if(value <= rates.health + rates.ammo + rates.shield + rates.firepower)
		kind = PickupKind::Firepower;
	else
		return;

	const int size = GameConfig::kPickupSize;
	const int enemy_w = GameConfig::kEnemyWidth;

	// 掉落物同速下落，相对位置一旦确定就不会变，
	// 所以"两个道具叠在一起"只可能发生在生成瞬间——在这里避开就行。
	auto overlaps_existing = [&](int candidate_x) {
		for(const auto& other: pickups_)
		{
			const bool apart = (candidate_x + size <= other.x)
			    || (other.x + size <= candidate_x)
			    || (enemy_y + size <= other.y) || (other.y + size <= enemy_y);
			if(!apart)
				return true;
		}
		return false;
	};

	// 优先落在敌机正下方，被占了就往两侧找，一个宽度一个宽度地试
	const int base = std::clamp(enemy_x + enemy_w / 2 - size / 2, 0,
	                            GameConfig::kScreenWidth - size);

	const int max_offset = size * GameConfig::kPickupPlacementTries;
	for(int offset = 0; offset <= max_offset; offset += size)
	{
		const int candidates[2] = {base + offset, base - offset};
		for(int x: candidates)
		{
			if(x < 0 || x > GameConfig::kScreenWidth - size)
				continue;
			if(overlaps_existing(x))
				continue;
			pickups_.emplace_back(x, enemy_y, kind);
			return;
		}
	}

	// 附近实在挤不下（例如道具刷得太密），这次就不掉了
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
		                              return p.y >= GameConfig::kScreenHeight;
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
	const int grab_y = player_y_ + GameConfig::kPlayerHeight - grab_h; // 底边对齐

	for(auto it = pickups_.begin(); it != pickups_.end();)
	{
		const bool no_overlap = (grab_x + grab_w <= it->x) || (it->x + size <= grab_x)
		    || (grab_y + grab_h <= it->y) || (it->y + size <= grab_y);

		if(no_overlap)
		{
			++it;
			continue;
		}

		switch(it->kind)
		{
		case PickupKind::Health:
			life_ += GameConfig::kHealthPickupLife;
			break;
		case PickupKind::Ammo:
			bullets_remaining_ += GameConfig::kAmmoPickupBullets;
			break;
		case PickupKind::Shield:
			// 护盾最多一层：已有盾时再捡只给补偿分，避免变成可叠加的"血包加强版"
			if(player_shield_)
				score_ += GameConfig::kDuplicateShieldScore;
			else
				player_shield_ = true;
			break;
		case PickupKind::Firepower:
			// 重复拾取刷新时长，不累加
			firepower_frames_left_ = GameConfig::kFirepowerFrames;
			break;
		}

		it = pickups_.erase(it);
	}
}

} // namespace thunder
