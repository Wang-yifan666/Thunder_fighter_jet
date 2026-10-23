// SDL3 前端绘制：全部由 SessionSnapshot 驱动，不接触 GameWorld 内部。
// 图形为纯色块 + 内置 5x7 点阵字体，不依赖 SDL_ttf。

#include "sdl_render.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "bitmap_font.h"
#include "game/game_config.h"

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
constexpr SDL_Color kText{235, 235, 240, 255};
constexpr SDL_Color kTextDim{150, 150, 165, 255};
constexpr SDL_Color kTextAccent{255, 214, 64, 255};
constexpr SDL_Color kOverlay{0, 0, 0, 170};

void Fill(SDL_Renderer* renderer, float x, float y, float w, float h,
          SDL_Color color)
{
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_FRect rect{x, y, w, h};
	SDL_RenderFillRect(renderer, &rect);
}

void Blend(SDL_Renderer* renderer, float x, float y, float w, float h,
           SDL_Color color)
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	Fill(renderer, x, y, w, h, color);
}

void CenterText(SDL_Renderer* renderer, int width, int y, int scale,
                const std::string& text, SDL_Color color)
{
	const int x = (width - BitmapFont::TextWidth(text, scale)) / 2;
	BitmapFont::DrawText(renderer, x, y, scale, text, color);
}

std::string PaddedNumber(int value, int width)
{
	std::ostringstream out;
	out << std::setw(width) << std::setfill('0') << value;
	return out.str();
}

void DrawStars(SDL_Renderer* renderer, const SdlRender::Layout& layout)
{
	const int grid_w = GameConfig::kScreenWidth * layout.cell;
	const int grid_h = GameConfig::kScreenHeight * layout.cell;

	for(int i = 0; i < 72; ++i)
	{
		// 固定散列保证星星位置稳定（不依赖游戏随机数）
		const std::uint32_t hash =
		    static_cast<std::uint32_t>(i) * 2654435761u;
		const int x = layout.grid_x
		    + static_cast<int>(hash % static_cast<std::uint32_t>(grid_w));
		const int y = layout.grid_y + static_cast<int>(
		                                  (hash / 7919u)
		                                  % static_cast<std::uint32_t>(grid_h));
		const bool bright = (i % 9) == 0;
		const float size = bright ? 2.0f : 1.0f;

		Fill(renderer, static_cast<float>(x), static_cast<float>(y), size,
		     size, bright ? kStarBright : kStar);
	}
}

void DrawEnemy(SDL_Renderer* renderer, const SdlRender::Layout& layout,
               const thunder::EntityView& enemy)
{
	const float cell = static_cast<float>(layout.cell);
	const float x = static_cast<float>(layout.grid_x) + enemy.x * cell;
	const float y = static_cast<float>(layout.grid_y) + enemy.y * cell;
	const float w = enemy.width * cell - 1.0f;
	const float h = enemy.height * cell - 1.0f;

	// 机身 + 左右翼 + 引擎光点
	Fill(renderer, x + 3.0f, y + 1.0f, w - 6.0f, h - 1.0f, kEnemyBody);
	Fill(renderer, x, y + h * 0.45f, 3.0f, h * 0.55f, kEnemyWing);
	Fill(renderer, x + w - 3.0f, y + h * 0.45f, 3.0f, h * 0.55f,
	     kEnemyWing);
	Fill(renderer, x + w * 0.5f - 1.0f, y + h - 2.0f, 2.0f, 2.0f,
	     kEnemyCore);
}

void DrawBullet(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::EntityView& bullet)
{
	const float cell = static_cast<float>(layout.cell);
	const float cx = static_cast<float>(layout.grid_x)
	    + bullet.x * cell + cell * 0.5f;
	const float cy = static_cast<float>(layout.grid_y)
	    + bullet.y * cell + cell * 0.5f;

	Fill(renderer, cx - 2.0f, cy - 4.0f, 4.0f, 8.0f, kBullet);
	Fill(renderer, cx - 1.0f, cy - 3.0f, 2.0f, 5.0f, kBulletCore);
}

void DrawPlayer(SDL_Renderer* renderer, const SdlRender::Layout& layout,
                const thunder::WorldSnapshot& world)
{
	const float cell = static_cast<float>(layout.cell);
	const float x = static_cast<float>(layout.grid_x)
	    + world.player_x * cell;
	const float y = static_cast<float>(layout.grid_y)
	    + world.player_y * cell;
	const float w = GameConfig::kPlayerWidth * cell - 1.0f;
	const float h = cell - 1.0f;

	const SDL_Color body = world.invincible ? kTextDim : kPlayer;
	const SDL_Color wing = world.invincible ? kTextDim : kPlayerWing;

	// 主体（中间一格）+ 两侧机翼 + 驾驶舱
	Fill(renderer, x + w / 3.0f, y, w / 3.0f, h, body);
	Fill(renderer, x, y + h * 0.55f, w / 3.0f, h * 0.45f, wing);
	Fill(renderer, x + w * 2.0f / 3.0f, y + h * 0.55f, w / 3.0f,
	     h * 0.45f, wing);
	Fill(renderer, x + w / 2.0f - 1.0f, y + 1.0f, 2.0f, 2.0f, kPlayerCore);
}

void DrawWorld(SDL_Renderer* renderer, const thunder::WorldSnapshot& world,
               const SdlRender::Layout& layout, bool paused)
{
	const int cell = layout.cell;
	const int grid_w = GameConfig::kScreenWidth * cell;
	const int grid_h = GameConfig::kScreenHeight * cell;

	Fill(renderer, static_cast<float>(layout.grid_x),
	     static_cast<float>(layout.grid_y), static_cast<float>(grid_w),
	     static_cast<float>(grid_h), kGridBg);

	DrawStars(renderer, layout);

	// 网格线
	SDL_SetRenderDrawColor(renderer, kGridLine.r, kGridLine.g, kGridLine.b,
	                       kGridLine.a);
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

	for(const auto& enemy: world.enemies)
		DrawEnemy(renderer, layout, enemy);

	for(const auto& bullet: world.bullets)
		DrawBullet(renderer, layout, bullet);

	DrawPlayer(renderer, layout, world);

	if(paused)
	{
		Blend(renderer, static_cast<float>(layout.grid_x),
		      static_cast<float>(layout.grid_y), static_cast<float>(grid_w),
		      static_cast<float>(grid_h), kOverlay);
		CenterText(renderer, layout.window_width, 200, 4, "PAUSED",
		           kTextAccent);
	}
}

void DrawHud(SDL_Renderer* renderer, const thunder::WorldSnapshot& world,
             int best, int fps, const SdlRender::Layout& layout)
{
	const int enemies = static_cast<int>(world.enemies.size())
	    + world.pending_enemies;

	const std::string line1 = "SCORE " + PaddedNumber(world.score, 10)
	    + "   LEVEL " + std::to_string(world.level) + "   TIME "
	    + std::to_string(world.elapsed_seconds) + "S";

	const std::string line2 = "LIFE " + std::to_string(world.life)
	    + "   AMMO " + std::to_string(world.bullets_remaining) + "   ENEMIES "
	    + std::to_string(enemies) + "   BEST " + std::to_string(best)
	    + "   FPS " + std::to_string(fps);

	BitmapFont::DrawText(renderer, layout.grid_x, 8, 2, line1, kTextAccent);
	BitmapFont::DrawText(renderer, layout.grid_x, 28, 2, line2, kText);
}

void DrawHint(SDL_Renderer* renderer, const SdlRender::Layout& layout,
              const std::string& text)
{
	CenterText(renderer, layout.window_width, layout.window_height - 26, 1,
	           text, kTextDim);
}

void DrawMenu(SDL_Renderer* renderer, const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 120, 4, "THUNDER FIGHTER",
	           kTextAccent);
	CenterText(renderer, layout.window_width, 176, 2, "SDL3 BUILD", kTextDim);

	CenterText(renderer, layout.window_width, 250, 2, "1  START GAME", kText);
	CenterText(renderer, layout.window_width, 290, 2, "2  LEADERBOARD", kText);
	CenterText(renderer, layout.window_width, 330, 2, "3  QUIT", kText);

	DrawHint(renderer, layout, "PRESS 1 / 2 / 3");
}

void DrawDifficulty(SDL_Renderer* renderer, const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 120, 3, "SELECT DIFFICULTY",
	           kTextAccent);

	CenterText(renderer, layout.window_width, 230, 2, "1  EASY", kText);
	CenterText(renderer, layout.window_width, 270, 2, "2  NORMAL", kText);
	CenterText(renderer, layout.window_width, 310, 2, "3  HARD", kText);

	DrawHint(renderer, layout, "B  BACK");
}

void DrawLeaderboard(SDL_Renderer* renderer,
                     const thunder::SessionSnapshot& snapshot,
                     const SdlRender::Layout& layout)
{
	CenterText(renderer, layout.window_width, 90, 3, "LEADERBOARD TOP 5",
	           kTextAccent);

	int y = 180;
	for(size_t i = 0; i < snapshot.leaderboard.size(); ++i)
	{
		const std::string row = std::to_string(i + 1) + ".  "
		    + std::to_string(snapshot.leaderboard[i]);
		CenterText(renderer, layout.window_width, y, 2, row, kText);
		y += 36;
	}

	DrawHint(renderer, layout, "B  BACK");
}

void DrawGameOver(SDL_Renderer* renderer,
                  const thunder::SessionSnapshot& snapshot,
                  const SdlRender::Layout& layout)
{
	Blend(renderer, 120, 140, layout.window_width - 240, 200,
	      SDL_Color{0, 0, 0, 200});

	CenterText(renderer, layout.window_width, 160, 4, "GAME OVER",
	           kEnemyBody);

	const std::string score = "SCORE " + std::to_string(snapshot.world.score)
	    + "  +AMMO " + std::to_string(
	                         snapshot.world.bullets_remaining
	                         * GameConfig::kEndGameBulletBonus);
	CenterText(renderer, layout.window_width, 220, 2, score, kTextAccent);

	std::string rank_text;
	if(snapshot.new_record)
		rank_text = "NEW RECORD!";
	else if(snapshot.entered_top5)
		rank_text = "TOP 5  RANK " + std::to_string(snapshot.rank);
	else
		rank_text = "NO TOP 5";
	CenterText(renderer, layout.window_width, 260, 2, rank_text, kText);

	CenterText(renderer, layout.window_width, 300, 2, "R RESTART   Q QUIT",
	           kTextDim);
}

} // namespace

namespace SdlRender
{

void Draw(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot,
          int fps)
{
	const Layout layout;

	Fill(renderer, 0, 0, static_cast<float>(layout.window_width),
	     static_cast<float>(layout.window_height), kBg);

	const int best = snapshot.leaderboard.empty()
	    ? 0
	    : snapshot.leaderboard.front();

	switch(snapshot.screen)
	{
	case thunder::Screen::Menu:
		DrawMenu(renderer, layout);
		break;

	case thunder::Screen::DifficultySelect:
		DrawDifficulty(renderer, layout);
		break;

	case thunder::Screen::Leaderboard:
		DrawLeaderboard(renderer, snapshot, layout);
		break;

	case thunder::Screen::Playing:
		DrawWorld(renderer, snapshot.world, layout, snapshot.paused);
		DrawHud(renderer, snapshot.world, best, fps, layout);
		DrawHint(renderer, layout,
		         "WASD MOVE   SPACE FIRE   P PAUSE   Q MENU   F11 FULLSCREEN   ESC QUIT");
		break;

	case thunder::Screen::GameOver:
		DrawWorld(renderer, snapshot.world, layout, false);
		DrawHud(renderer, snapshot.world, best, fps, layout);
		DrawGameOver(renderer, snapshot, layout);
		break;
	}
}

} // namespace SdlRender
