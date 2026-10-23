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
constexpr SDL_Color kPlayer{250, 210, 70, 255};
constexpr SDL_Color kPlayerCore{40, 40, 48, 255};
constexpr SDL_Color kEnemy{220, 60, 60, 255};
constexpr SDL_Color kBullet{255, 120, 200, 255};
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

void DrawWorld(SDL_Renderer* renderer, const thunder::WorldSnapshot& world,
               const SdlRender::Layout& layout, bool paused)
{
	const int cell = layout.cell;
	const int grid_w = GameConfig::kScreenWidth * cell;
	const int grid_h = GameConfig::kScreenHeight * cell;

	Fill(renderer, static_cast<float>(layout.grid_x),
	     static_cast<float>(layout.grid_y), static_cast<float>(grid_w),
	     static_cast<float>(grid_h), kGridBg);

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

	// 敌机
	for(const auto& enemy: world.enemies)
	{
		Fill(renderer,
		     static_cast<float>(layout.grid_x + enemy.x * cell),
		     static_cast<float>(layout.grid_y + enemy.y * cell),
		     static_cast<float>(enemy.width * cell - 1),
		     static_cast<float>(enemy.height * cell - 1), kEnemy);
	}

	// 子弹
	for(const auto& bullet: world.bullets)
	{
		const float size = 4.0f;
		Fill(renderer,
		     static_cast<float>(layout.grid_x + bullet.x * cell) + (cell - size) / 2.0f,
		     static_cast<float>(layout.grid_y + bullet.y * cell) + (cell - size) / 2.0f,
		     size, size, kBullet);
	}

	// 玩家：3 格机身 + 中央驾驶舱
	const float px = static_cast<float>(layout.grid_x + world.player_x * cell);
	const float py = static_cast<float>(layout.grid_y + world.player_y * cell);
	const float pw = static_cast<float>(GameConfig::kPlayerWidth * cell - 1);
	const float ph = static_cast<float>(cell - 1);
	Fill(renderer, px, py, pw, ph, world.invincible ? kTextDim : kPlayer);
	Fill(renderer, px + pw / 3.0f, py + 1.0f, pw / 3.0f, ph - 2.0f,
	     kPlayerCore);

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

	CenterText(renderer, layout.window_width, 160, 4, "GAME OVER", kEnemy);

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
		         "WASD MOVE   SPACE FIRE   P PAUSE   Q MENU   ESC QUIT");
		break;

	case thunder::Screen::GameOver:
		DrawWorld(renderer, snapshot.world, layout, false);
		DrawHud(renderer, snapshot.world, best, fps, layout);
		DrawGameOver(renderer, snapshot, layout);
		break;
	}
}

} // namespace SdlRender
