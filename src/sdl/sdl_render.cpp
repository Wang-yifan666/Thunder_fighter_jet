// SDL3 前端绘制：全部由 SessionSnapshot 驱动，不接触 GameWorld 内部。
// 图形为纯色块；文字优先 Ubuntu Mono（SDL3_ttf），否则回退内置点阵字体。

#include "sdl_render.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "game/game_config.h"
#include "game/game_text.h"
#include "sdl_font.h"
#include "sprite_data.h"

namespace
{

constexpr SDL_Color kBg{12, 12, 16, 255};
constexpr SDL_Color kGridBg{18, 18, 24, 255};
constexpr SDL_Color kGridLine{30, 30, 40, 255};
constexpr SDL_Color kStar{58, 58, 76, 255};
constexpr SDL_Color kStarBright{96, 96, 120, 255};
constexpr SDL_Color kPlayer{250, 210, 70, 255};
constexpr SDL_Color kPlayerWing{196, 148, 32, 255};
constexpr SDL_Color kPlayerCore{40, 40, 48, 255};
constexpr SDL_Color kEnemyBody{220, 60, 60, 255};
constexpr SDL_Color kEnemyWing{150, 34, 42, 255};
constexpr SDL_Color kEnemyCore{255, 150, 90, 255};
constexpr SDL_Color kBullet{255, 120, 200, 255};
constexpr SDL_Color kBulletCore{255, 240, 250, 255};
constexpr SDL_Color kShockwave{150, 230, 255, 255};
constexpr SDL_Color kHealthBody{232, 64, 64, 255};
constexpr SDL_Color kHealthMark{255, 245, 240, 255};
constexpr SDL_Color kAmmoBody{250, 196, 32, 255};
constexpr SDL_Color kShieldBody{64, 176, 232, 255};
constexpr SDL_Color kFirepowerBody{138, 84, 220, 255};
constexpr SDL_Color kDarkMark{40, 40, 52, 255};
constexpr SDL_Color kText{235, 235, 240, 255};
constexpr SDL_Color kTextDim{150, 150, 165, 255};
constexpr SDL_Color kTextAccent{255, 214, 64, 255};
constexpr SDL_Color kOverlay{0, 0, 0, 170};

void Fill(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color color)
{
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_FRect rect{x, y, w, h};
	SDL_RenderFillRect(renderer, &rect);
}

void Blend(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color color)
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	Fill(renderer, x, y, w, h, color);
}

void CenterText(SDL_Renderer* renderer, int width, int y, int scale, const std::string& text,
                SDL_Color color)
{
	const int x = (width - SdlFont::TextWidth(text, scale)) / 2;
	SdlFont::DrawText(renderer, x, y, scale, text, color);
}

std::string PaddedNumber(int value, int width)
{
	std::ostringstream out;
	out << std::setw(width) << std::setfill('0') << value;
	return out.str();
}

void DrawStars(SDL_Renderer* renderer, const SdlRender::Layout& layout, double seconds)
{
	const int grid_w = GameConfig::kScreenWidth * layout.cell;
	const int grid_h = GameConfig::kScreenHeight * layout.cell;

	for(int i = 0; i < 72; ++i)
	{
		// 固定散列保证初始位置稳定；按深度不同速度缓慢下落，形成视差
		const std::uint32_t hash = static_cast<std::uint32_t>(i) * 2654435761u;
		const int x = layout.grid_x + static_cast<int>(hash % static_cast<std::uint32_t>(grid_w));
		const int depth = (i % 3) + 1;
		const double speed = 8.0 * depth;
		const double base_y =
		    static_cast<double>(static_cast<int>(hash / 7919u) % static_cast<std::uint32_t>(grid_h));
		const int y = layout.grid_y
		    + static_cast<int>(std::fmod(base_y + seconds * speed, static_cast<double>(grid_h)));
		const bool bright = (i % 9) == 0;
		const float size = bright ? 2.0f : 1.0f;

		Fill(renderer, static_cast<float>(x), static_cast<float>(y), size, size,
		     bright ? kStarBright : kStar);
	}
}


/// 一张按需创建、随后复用的精灵贴图。
struct SpriteSlot
{
	SDL_Texture* texture = nullptr;
	SDL_Renderer* owner = nullptr;
};

/// 由内嵌数据创建贴图；数据长度不符或创建失败时返回 nullptr，
/// 调用方据此回退到色块精灵。
SDL_Texture* AcquireTexture(SpriteSlot& slot, SDL_Renderer* renderer, const unsigned char* pixels,
                            std::size_t size, int width, int height)
{
	if(slot.texture != nullptr && slot.owner == renderer)
		return slot.texture;

	if(slot.texture != nullptr)
	{
		SDL_DestroyTexture(slot.texture);
		slot.texture = nullptr;
		slot.owner = nullptr;
	}

	const std::size_t expected =
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
	if(size != expected)
		return nullptr;

	SDL_Texture* texture =
	    SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
	if(texture == nullptr)
		return nullptr;

	if(!SDL_UpdateTexture(texture, nullptr, pixels, width * 4))
	{
		SDL_DestroyTexture(texture);
		return nullptr;
	}

	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	slot.texture = texture;
	slot.owner = renderer;
	return texture;
}

SDL_Texture* PlayerTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kPlayerPixels, SpriteData::kPlayerPixelsSize,
	                      View::kPlayerSpriteSize, View::kPlayerSpriteSize);
}

SDL_Texture* EnemyTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kEnemyPixels, SpriteData::kEnemyPixelsSize,
	                      View::kEnemySpriteWidth, View::kEnemySpriteHeight);
}

SDL_Texture* HealthTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kHealthPixels, SpriteData::kHealthPixelsSize,
	                      View::kPickupSpriteSize, View::kPickupSpriteSize);
}

SDL_Texture* AmmoTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kAmmoPixels, SpriteData::kAmmoPixelsSize,
	                      View::kPickupSpriteSize, View::kPickupSpriteSize);
}

SDL_Texture* EliteTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kElitePixels, SpriteData::kElitePixelsSize,
	                      View::kEliteSpriteWidth, View::kEliteSpriteHeight);
}

SDL_Texture* ShieldPackTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kShieldPackPixels,
	                      SpriteData::kShieldPackPixelsSize, View::kPickupSpriteSize,
	                      View::kPickupSpriteSize);
}

SDL_Texture* FirepowerTexture(SDL_Renderer* renderer)
{
	static SpriteSlot slot;
	return AcquireTexture(slot, renderer, SpriteData::kFirepowerPixels,
	                      SpriteData::kFirepowerPixelsSize, View::kPickupSpriteSize,
	                      View::kPickupSpriteSize);
}

/// 护盾泡：小怪 32x32、精英 / 玩家 48x48
SDL_Texture* ShieldBubbleTexture(SDL_Renderer* renderer, bool big)
{
	static SpriteSlot small_slot;
	static SpriteSlot big_slot;
	if(big)
	{
		return AcquireTexture(big_slot, renderer, SpriteData::kShieldBubbleBigPixels,
		                      SpriteData::kShieldBubbleBigPixelsSize, View::kShieldBubbleBigSize,
		                      View::kShieldBubbleBigSize);
	}
	return AcquireTexture(small_slot, renderer, SpriteData::kShieldBubblePixels,
	                      SpriteData::kShieldBubblePixelsSize, View::kShieldBubbleSize,
	                      View::kShieldBubbleSize);
}

/// 掉落物：带轻微上下浮动，便于和敌机区分（纯视觉，不影响判定）
void DrawPickup(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::PickupView& item, double seconds)
{
	const float cell = static_cast<float>(layout.cell);
	const float bob = static_cast<float>(std::sin(seconds * 2.4) * 1.2);
	const View::Rect box = View::PickupRect(layout, item.x, item.y, item.width, item.height, bob);
	const float x = box.x;
	const float y = box.y;
	const float w = box.w;
	const float h = box.h;

	SDL_Texture* texture = nullptr;
	switch(item.kind)
	{
	case PickupKind::Health:
		texture = HealthTexture(renderer);
		break;
	case PickupKind::Ammo:
		texture = AmmoTexture(renderer);
		break;
	case PickupKind::Shield:
		texture = ShieldPackTexture(renderer);
		break;
	case PickupKind::Firepower:
		texture = FirepowerTexture(renderer);
		break;
	}

	if(texture != nullptr)
	{
		const SDL_FRect dst{x, y, w, h};
		SDL_RenderTexture(renderer, texture, nullptr, &dst);
		return;
	}

	// 回退：四种道具各画一个色块 + 记号
	const float pad = cell * 0.5f;
	SDL_Color body = kHealthBody;
	switch(item.kind)
	{
	case PickupKind::Health:
		body = kHealthBody;
		break;
	case PickupKind::Ammo:
		body = kAmmoBody;
		break;
	case PickupKind::Shield:
		body = kShieldBody;
		break;
	case PickupKind::Firepower:
		body = kFirepowerBody;
		break;
	}
	const SDL_Color mark = (item.kind == PickupKind::Health) ? kHealthMark : kDarkMark;

	Fill(renderer, x + pad, y + pad, w - pad * 2, h - pad * 2, body);

	const float cx = x + w * 0.5f;
	const float cy = y + h * 0.5f;
	if(item.kind == PickupKind::Health)
	{
		const float bar = (w - pad * 2) * 0.28f;
		Fill(renderer, cx - bar * 0.5f, cy - (h - pad * 2) * 0.5f + 2.0f, bar, h - pad * 2 - 4.0f,
		     mark);
		Fill(renderer, cx - (w - pad * 2) * 0.5f + 2.0f, cy - bar * 0.5f, w - pad * 2 - 4.0f, bar,
		     mark);
	}
	else
	{
		Fill(renderer, cx - 5.0f, cy - 4.0f, 10.0f, 8.0f, mark);
	}
}

/// 用短线段拼一个圆环（SDL 没有直接画圆的接口）
void StrokeRing(SDL_Renderer* renderer, float center_x, float center_y, float radius,
                SDL_Color color, int alpha)
{
	if(radius <= 0.5f)
		return;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, static_cast<Uint8>(alpha));

	const int segments = std::max(16, static_cast<int>(radius));
	const float step = 6.2831853f / static_cast<float>(segments);
	float prev_x = center_x + radius;
	float prev_y = center_y;
	for(int i = 1; i <= segments; ++i)
	{
		const float angle = step * static_cast<float>(i);
		const float x = center_x + std::cos(angle) * radius;
		const float y = center_y + std::sin(angle) * radius;
		SDL_RenderLine(renderer, prev_x, prev_y, x, y);
		prev_x = x;
		prev_y = y;
	}
}

/// 纯视觉特效：命中火花 + 破盾冲击波（不参与任何判定）
void DrawEffects(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                 const std::vector<thunder::EffectView>& effects)
{
	const float cell = static_cast<float>(layout.cell);

	for(const auto& fx: effects)
	{
		const float progress = (fx.life > 0)
		    ? static_cast<float>(fx.age) / static_cast<float>(fx.life)
		    : 1.0f;
		const float cx = View::CellX(layout, fx.x) + cell * 0.5f;
		const float cy = View::CellY(layout, fx.y) + cell * 0.5f;

		if(fx.kind == EffectKind::Shockwave)
		{
			const int alpha = static_cast<int>(220.0f * (1.0f - progress));
			StrokeRing(renderer, cx, cy, fx.radius * cell * progress, kShockwave, alpha);
			StrokeRing(renderer, cx, cy, fx.radius * cell * progress * 0.7f, kShockwave, alpha / 2);
			continue;
		}

		const float fade = 1.0f - progress;
		// 火花是小十字，贴在命中点上，很快就淡掉——别让它看起来像"没打完的子弹"
		const float len = 1.0f + 3.0f * fade;
		const int alpha = static_cast<int>(220.0f * fade);
		const SDL_Color spark{255, 255, 255, static_cast<Uint8>(alpha)};
		Blend(renderer, cx - len, cy - 1.0f, len * 2.0f, 2.0f, spark);
		Blend(renderer, cx - 1.0f, cy - len, 2.0f, len * 2.0f, spark);
	}
}

void DrawEnemy(SDL_Renderer* renderer, const SdlRender::Layout& layout,
               const thunder::EnemyView& enemy, double seconds)
{
	const bool elite = (enemy.kind == EnemyKind::Elite);
	const View::Rect box = View::EnemyRect(layout, enemy.x, enemy.y, enemy.width, enemy.height);
	const float x = box.x;
	const float y = box.y;
	const float w = box.w - 1.0f;
	const float h = box.h - 1.0f;

	// 破盾无敌期闪烁：子弹会穿过去，这里必须能一眼看出"暂时打不动"
	const bool flicker = enemy.invulnerable && std::fmod(seconds, 0.16) >= 0.08;

	SDL_Texture* texture = elite ? EliteTexture(renderer) : EnemyTexture(renderer);
	if(texture != nullptr)
	{
		if(flicker)
			SDL_SetTextureAlphaMod(texture, 110);
		const SDL_FRect dst{box.x, box.y, box.w, box.h};
		SDL_RenderTexture(renderer, texture, nullptr, &dst);
		if(flicker)
			SDL_SetTextureAlphaMod(texture, 255);
	}
	else
	{
		// 回退：机身 + 左右翼 + 引擎光点
		Fill(renderer, x + 3.0f, y + 1.0f, w - 6.0f, h - 1.0f, kEnemyBody);
		Fill(renderer, x, y + h * 0.45f, 3.0f, h * 0.55f, kEnemyWing);
		Fill(renderer, x + w - 3.0f, y + h * 0.45f, 3.0f, h * 0.55f, kEnemyWing);
		Fill(renderer, x + w * 0.5f - 1.0f, y + h - 2.0f, 2.0f, 2.0f, kEnemyCore);
	}

	// 护盾泡画在敌机之上：一圈青光，明确告诉玩家"先打盾"
	if(enemy.shielded)
	{
		if(SDL_Texture* bubble = ShieldBubbleTexture(renderer, elite))
		{
			const int size =
			    elite ? View::kShieldBubbleBigSize : View::kShieldBubbleSize;
			const View::Rect ring =
			    View::EnemyShieldRect(layout, enemy.x, enemy.y, enemy.width, enemy.height, size);
			const SDL_FRect dst{ring.x, ring.y, ring.w, ring.h};
			SDL_RenderTexture(renderer, bubble, nullptr, &dst);
		}
	}
}

void DrawBullet(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::EntityView& bullet)
{
	const View::Rect body = View::BulletRect(layout, bullet.x, bullet.y, 4.0f, 8.0f);
	const float cx = body.x + body.w * 0.5f;
	const float cy = body.y + body.h * 0.5f;

	// 尾焰：现在每 2 帧才发一发，子弹之间本来就隔着一格；
	// 往"后方"补一小段渐隐，弹流看起来才不像碎掉的线段。
	// 只朝后方延伸——前方边缘仍严格等于 1 格命中框，不会出现"看着打到却没算"。
	const SDL_Color tail{255, 120, 200, 90};
	Blend(renderer, body.x, body.y + body.h, body.w, 5.0f, tail);

	Fill(renderer, body.x, body.y, body.w, body.h, kBullet);
	Fill(renderer, cx - 1.0f, cy - 3.0f, 2.0f, 5.0f, kBulletCore);
}

void DrawPlayer(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::WorldSnapshot& world, double seconds)
{
	// 无敌期间闪烁（纯视觉；不影响碰撞判定）
	if(world.invincible && std::fmod(seconds, 0.24) >= 0.16)
		return;

	const View::Rect hitbox = View::CellRect(layout, world.player_x, world.player_y,
	                                         GameConfig::kPlayerWidth, GameConfig::kPlayerHeight);
	const float x = hitbox.x;
	const float y = hitbox.y;
	const float w = hitbox.w - 1.0f;
	const float h = hitbox.h - 1.0f;

	if(SDL_Texture* texture = PlayerTexture(renderer))
	{
		const View::Rect box = View::PlayerRect(layout, world.player_x, world.player_y);
		const SDL_FRect dst{box.x, box.y, box.w, box.h};

		// 无敌时压暗，与色块版本的观感保持一致
		if(world.invincible)
			SDL_SetTextureColorMod(texture, kTextDim.r, kTextDim.g, kTextDim.b);
		SDL_RenderTexture(renderer, texture, nullptr, &dst);
		if(world.invincible)
			SDL_SetTextureColorMod(texture, 255, 255, 255);
	}
	else
	{
		const SDL_Color body = world.invincible ? kTextDim : kPlayer;
		const SDL_Color wing = world.invincible ? kTextDim : kPlayerWing;

		// 回退：主体（中间一格）+ 两侧机翼 + 驾驶舱
		Fill(renderer, x + w / 3.0f, y, w / 3.0f, h, body);
		Fill(renderer, x, y + h * 0.55f, w / 3.0f, h * 0.45f, wing);
		Fill(renderer, x + w * 2.0f / 3.0f, y + h * 0.55f, w / 3.0f, h * 0.45f, wing);
		Fill(renderer, x + w / 2.0f - 1.0f, y + 1.0f, 2.0f, 2.0f, kPlayerCore);
	}

	// 玩家护盾泡：和敌机用同一张青光贴图，读到"这层盾会挡住一次撞击"
	if(world.shielded)
	{
		if(SDL_Texture* bubble = ShieldBubbleTexture(renderer, true))
		{
			const View::Rect ring = View::PlayerShieldRect(layout, world.player_x, world.player_y);
			const SDL_FRect dst{ring.x, ring.y, ring.w, ring.h};
			SDL_RenderTexture(renderer, bubble, nullptr, &dst);
		}
	}
}

void DrawStatus(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::WorldSnapshot& world);

void DrawWorld(SDL_Renderer* renderer, const thunder::WorldSnapshot& world,
               const SdlRender::Layout& layout, bool paused, double seconds)
{
	const int cell = layout.cell;
	const int grid_w = GameConfig::kScreenWidth * cell;
	const int grid_h = GameConfig::kScreenHeight * cell;

	Fill(renderer, static_cast<float>(layout.grid_x), static_cast<float>(layout.grid_y),
	     static_cast<float>(grid_w), static_cast<float>(grid_h), kGridBg);

	DrawStars(renderer, layout, seconds);

	// 网格线
	SDL_SetRenderDrawColor(renderer, kGridLine.r, kGridLine.g, kGridLine.b, kGridLine.a);
	for(int x = 0; x <= GameConfig::kScreenWidth; ++x)
	{
		const float px = static_cast<float>(layout.grid_x + x * cell);
		SDL_RenderLine(renderer, px, static_cast<float>(layout.grid_y), px,
		               static_cast<float>(layout.grid_y + grid_h));
	}
	for(int y = 0; y <= GameConfig::kScreenHeight; ++y)
	{
		const float py = static_cast<float>(layout.grid_y + y * cell);
		SDL_RenderLine(renderer, static_cast<float>(layout.grid_x), py,
		               static_cast<float>(layout.grid_x + grid_w), py);
	}

	// 精灵允许超出格子（玩家贴图向上悬垂），因此裁剪到棋盘范围内
	const SDL_Rect clip{layout.grid_x, layout.grid_y, grid_w, grid_h};
	SDL_SetRenderClipRect(renderer, &clip);

	for(const auto& enemy: world.enemies)
		DrawEnemy(renderer, layout, enemy, seconds);

	for(const auto& bullet: world.bullets)
		DrawBullet(renderer, layout, bullet);

	for(const auto& item: world.pickups)
		DrawPickup(renderer, layout, item, seconds);

	DrawPlayer(renderer, layout, world, seconds);

	// 特效画在最上层：火花与冲击波要压过精灵才看得清
	DrawEffects(renderer, layout, world.effects);

	SDL_SetRenderClipRect(renderer, nullptr);

	DrawStatus(renderer, layout, world);

	if(paused)
	{
		Blend(renderer, static_cast<float>(layout.grid_x), static_cast<float>(layout.grid_y),
		      static_cast<float>(grid_w), static_cast<float>(grid_h), kOverlay);
		CenterText(renderer, layout.window_width, layout.grid_y + grid_h / 2 - 20, 4, "PAUSED",
		           kTextAccent);
	}
}

void DrawHud(SDL_Renderer* renderer, const thunder::WorldSnapshot& world, int best, int fps,
             const SdlRender::Layout& layout)
{
	const int enemies = static_cast<int>(world.enemies.size()) + world.pending_enemies;

	const std::string line1 = "SCORE " + PaddedNumber(world.score, 10) + "   LEVEL "
	    + std::to_string(world.level) + "   TIME " + std::to_string(world.elapsed_seconds) + "S";

	const std::string line2 = "LIFE " + std::to_string(world.life) + "   AMMO "
	    + std::to_string(world.bullets_remaining) + "   ENEMIES " + std::to_string(enemies)
	    + "   BEST " + std::to_string(best) + "   FPS " + std::to_string(fps);

	SdlFont::DrawText(renderer, layout.grid_x, 8, 2, line1, kTextAccent);
	SdlFont::DrawText(renderer, layout.grid_x, 28, 2, line2, kText);
}

/// 玩家状态角标：护盾 / 火力强化只在生效时显示，贴在棋盘左下角。
void DrawStatus(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::WorldSnapshot& world)
{
	int y = layout.grid_y + GameConfig::kScreenHeight * layout.cell - 12;

	if(world.firepower_seconds_left > 0)
	{
		SdlFont::DrawText(renderer, layout.grid_x + 4, y, 1,
		                  "POWER " + std::to_string(world.firepower_seconds_left) + "S",
		                  kTextAccent);
		y -= 10;
	}
	if(world.shielded)
	{
		SdlFont::DrawText(renderer, layout.grid_x + 4, y, 1, "SHIELD", kShockwave);
	}
}

void DrawHint(SDL_Renderer* renderer, const SdlRender::Layout& layout, const std::string& text)
{
	CenterText(renderer, layout.window_width, layout.window_height - 26, 1, text, kTextDim);
}

void DrawMenu(SDL_Renderer* renderer, const SdlRender::Layout& layout, bool cheats_enabled,
              bool cheat_notice)
{
	// 标题自动降档：内建点阵字体每字符 6*scale 像素，Ubuntu Mono 约 5*scale，
	// 40 个字符在点阵回退下 scale 4 会超出窗口，这里取能放下的最大档。
	const std::string title = "NAIDAN'S FEAR OF INSUFFICIENT FIREPOWER";
	int title_scale = 4;
	while(title_scale > 1 && SdlFont::TextWidth(title, title_scale) > layout.window_width - 24)
		--title_scale;

	CenterText(renderer, layout.window_width, 150, title_scale, title, kTextAccent);

	CenterText(renderer, layout.window_width, 290, 2, "1  START GAME", kText);
	CenterText(renderer, layout.window_width, 330, 2, "2  LEADERBOARD", kText);
	CenterText(renderer, layout.window_width, 370, 2, GameText::kTutorialMenu, kText);
	CenterText(renderer, layout.window_width, 410, 2, GameText::kHelpMenu, kText);
	CenterText(renderer, layout.window_width, 450, 2, "Q  QUIT", kText);

	// 作弊状态不常驻显示，只在切换后短暂提示一次
	if(cheat_notice)
	{
		CenterText(renderer, layout.window_width, 430, 2,
		           cheats_enabled ? "CHEAT MODE: ON" : "CHEAT MODE: OFF",
		           cheats_enabled ? kTextAccent : kTextDim);
	}

	DrawHint(renderer, layout, "PRESS 1 / 2 / 3 / 4 / Q");
}

void DrawDifficulty(SDL_Renderer* renderer, const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 150, 3, "SELECT DIFFICULTY", kTextAccent);

	CenterText(renderer, layout.window_width, 280, 2, "1  EASY", kText);
	CenterText(renderer, layout.window_width, 330, 2, "2  NORMAL", kText);
	CenterText(renderer, layout.window_width, 380, 2, "3  HARD", kText);

	DrawHint(renderer, layout, "B  BACK");
}

void DrawLeaderboard(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot,
                     const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 90, 3, "LEADERBOARD TOP 5", kTextAccent);

	int y = 180;
	for(size_t i = 0; i < snapshot.leaderboard.size(); ++i)
	{
		const std::string row =
		    std::to_string(i + 1) + ".  " + std::to_string(snapshot.leaderboard[i]);
		CenterText(renderer, layout.window_width, y, 2, row, kText);
		y += 36;
	}

	DrawHint(renderer, layout, "B  BACK");
}

void DrawGameOver(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot,
                  const SdlRender::Layout& layout)
{
	const int panel_height = 240;
	const int panel_y = (layout.window_height - panel_height) / 2;
	const int panel_x = 120;

	Blend(renderer, static_cast<float>(panel_x), static_cast<float>(panel_y),
	      static_cast<float>(layout.window_width - panel_x * 2), static_cast<float>(panel_height),
	      SDL_Color{0, 0, 0, 200});

	CenterText(renderer, layout.window_width, panel_y + 20, 4, "GAME OVER", kEnemyBody);

	const std::string score = "SCORE " + std::to_string(snapshot.world.score) + "  +AMMO "
	    + std::to_string(snapshot.world.bullets_remaining * GameConfig::kEndGameBulletBonus);
	CenterText(renderer, layout.window_width, panel_y + 80, 2, score, kTextAccent);

	std::string rank_text;
	if(snapshot.new_record)
		rank_text = "NEW RECORD!";
	else if(snapshot.entered_top5)
		rank_text = "TOP 5  RANK " + std::to_string(snapshot.rank);
	else
		rank_text = "NO TOP 5";
	CenterText(renderer, layout.window_width, panel_y + 120, 2, rank_text, kText);

	CenterText(renderer, layout.window_width, panel_y + 170, 2, "R RESTART   Q QUIT", kTextDim);
}

/// 玩法说明：一页讲清弹药、生命、升级、精英、护盾与四种补给。
void DrawHelp(SDL_Renderer* renderer, const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 40, 3, GameText::kHelpTitle, kTextAccent);

	const char* lines[] = {
	    GameText::kHelpLine1, GameText::kHelpLine2, GameText::kHelpLine3,
	    GameText::kHelpLine4, GameText::kHelpLine5, GameText::kHelpLine6,
	    GameText::kHelpLine7, GameText::kHelpLine8,
	};
	int y = 110;
	for(const char* line: lines)
	{
		CenterText(renderer, layout.window_width, y, 1, line, kText);
		y += 34;
	}

	DrawHint(renderer, layout, GameText::kHelpBack);
}

/// 教程：底部一条提示带，按步骤换文案；上方照常画世界（玩家、敌机、补给）。
void DrawTutorial(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot,
                  const SdlRender::Layout& layout, double seconds)
{
	DrawWorld(renderer, snapshot.world, layout, false, seconds);

	const char* line = "";
	const char* hint = "";
	switch(snapshot.tutorial_step)
	{
	case TutorialStep::Move:
		line = GameText::kTutorialMove;
		hint = GameText::kTutorialMoveHint;
		break;
	case TutorialStep::Fire:
		line = GameText::kTutorialFire;
		hint = GameText::kTutorialFireHint;
		break;
	case TutorialStep::Kill:
		line = GameText::kTutorialKill;
		hint = GameText::kTutorialKillHint;
		break;
	case TutorialStep::PickupHealth:
		line = GameText::kTutorialPickupHealth;
		hint = GameText::kTutorialPickupHealthHint;
		break;
	case TutorialStep::PickupAmmo:
		line = GameText::kTutorialPickupAmmo;
		hint = GameText::kTutorialPickupAmmoHint;
		break;
	case TutorialStep::PickupShield:
		line = GameText::kTutorialPickupShield;
		hint = GameText::kTutorialPickupShieldHint;
		break;
	case TutorialStep::PickupFirepower:
		line = GameText::kTutorialPickupFirepower;
		hint = GameText::kTutorialPickupFirepowerHint;
		break;
	case TutorialStep::Done:
		line = GameText::kTutorialDone;
		break;
	}

	// 顶部/底部各压一条半透明带，中文压在棋盘上也看得清
	const float band = 30.0f;
	Blend(renderer, 0.0f, 0.0f, static_cast<float>(layout.window_width), band,
	      SDL_Color{0, 0, 0, 180});
	Blend(renderer, 0.0f, static_cast<float>(layout.window_height) - band,
	      static_cast<float>(layout.window_width), band, SDL_Color{0, 0, 0, 180});

	// 达成目标后换成"做得好！"，给玩家一个明确的完成反馈再切下一步
	if(snapshot.tutorial_step_done && snapshot.tutorial_step != TutorialStep::Done)
		CenterText(renderer, layout.window_width, 8, 2, GameText::kTutorialWellDone, kShockwave);
	else
		CenterText(renderer, layout.window_width, 8, 2, line, kTextAccent);
	if(hint[0] != '\0')
		CenterText(renderer, layout.window_width, layout.window_height - 24, 1, hint, kTextDim);

	CenterText(renderer, layout.window_width, layout.window_height - 54, 1, GameText::kTutorialSkip,
	           kTextDim);
}

/// 教程结束页：走完 / 跳过都到这里，只有走完才显示解锁。
void DrawTutorialDone(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot,
                      const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 120, 3, GameText::kTutorialDone, kTextAccent);

	if(snapshot.tutorial_newly_unlocked)
		CenterText(renderer, layout.window_width, 210, 2, GameText::kTutorialUnlock, kText);

	CenterText(renderer, layout.window_width, 300, 2, GameText::kTutorialStart, kText);
	CenterText(renderer, layout.window_width, 350, 2, GameText::kTutorialBack, kText);

	DrawHint(renderer, layout, "PRESS 1 / 2");
}

} // namespace

namespace SdlRender
{

void Draw(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot, int fps, double seconds)
{
	const Layout layout;

	Fill(renderer, 0, 0, static_cast<float>(layout.window_width),
	     static_cast<float>(layout.window_height), kBg);

	const int best = snapshot.leaderboard.empty() ? 0 : snapshot.leaderboard.front();

	switch(snapshot.screen)
	{
	case thunder::Screen::Menu:
		DrawMenu(renderer, layout, snapshot.cheats_enabled, snapshot.cheat_notice);
		break;

	case thunder::Screen::DifficultySelect:
		DrawDifficulty(renderer, layout);
		break;

	case thunder::Screen::Leaderboard:
		DrawLeaderboard(renderer, snapshot, layout);
		break;

	case thunder::Screen::Playing:
		DrawWorld(renderer, snapshot.world, layout, snapshot.paused, seconds);
		DrawHud(renderer, snapshot.world, best, fps, layout);
		DrawHint(renderer, layout,
		         "WASD MOVE   SPACE FIRE   P PAUSE   Q MENU   F11 FULLSCREEN   ESC QUIT");
		break;

	case thunder::Screen::Help:
		DrawHelp(renderer, layout);
		break;

	case thunder::Screen::Tutorial:
		DrawTutorial(renderer, snapshot, layout, seconds);
		break;

	case thunder::Screen::TutorialDone:
		DrawTutorialDone(renderer, snapshot, layout);
		break;

	case thunder::Screen::GameOver:
		DrawWorld(renderer, snapshot.world, layout, false, seconds);
		DrawHud(renderer, snapshot.world, best, fps, layout);
		DrawGameOver(renderer, snapshot, layout);
		break;
	}
}

} // namespace SdlRender
