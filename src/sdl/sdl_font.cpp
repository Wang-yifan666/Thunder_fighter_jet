// 文本绘制：优先 Ubuntu Mono（SDL3_ttf），不可用时回退到内置 5x7 点阵字体。

#include "sdl_font.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "bitmap_font.h"

#if defined(THUNDER_HAVE_SDL3_TTF)
#include <SDL3_ttf/SDL_ttf.h>
#endif

namespace
{

#if defined(THUNDER_HAVE_SDL3_TTF)

constexpr int kMaxScale = 4;
TTF_Font* g_fonts[kMaxScale + 1] = {};
bool g_ttf_ready = false;

int ClampScale(int scale)
{
	return std::clamp(scale, 1, kMaxScale);
}

int PixelSize(int scale)
{
	return 6 * scale + 6; // scale 1..4 -> 12 / 18 / 24 / 30 px
}

std::string FindFontFile()
{
	std::vector<std::string> candidates;

	if(const char* base = SDL_GetBasePath())
	{
		candidates.push_back(std::string(base)
		                     + "assets/fonts/UbuntuMono-Regular.ttf");
		candidates.push_back(std::string(base)
		                     + "../assets/fonts/UbuntuMono-Regular.ttf");
	}

	candidates.push_back("assets/fonts/UbuntuMono-Regular.ttf");
	candidates.push_back("../assets/fonts/UbuntuMono-Regular.ttf");

	for(const auto& path: candidates)
	{
		SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
		if(io != nullptr)
		{
			SDL_CloseIO(io);
			return path;
		}
	}

	return {};
}

void CloseFonts()
{
	for(auto& font: g_fonts)
	{
		if(font != nullptr)
		{
			TTF_CloseFont(font);
			font = nullptr;
		}
	}
	g_ttf_ready = false;
}

#endif // THUNDER_HAVE_SDL3_TTF

} // namespace

namespace SdlFont
{

bool Init()
{
#if defined(THUNDER_HAVE_SDL3_TTF)
	if(!TTF_Init())
	{
		std::fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
		return false;
	}

	const std::string path = FindFontFile();
	if(path.empty())
	{
		std::fprintf(stderr,
		             "Ubuntu Mono not found; fallback to built-in bitmap font\n");
		TTF_Quit();
		return false;
	}

	for(int scale = 1; scale <= kMaxScale; ++scale)
	{
		TTF_Font* font = TTF_OpenFont(path.c_str(),
		                              static_cast<float>(PixelSize(scale)));
		if(font == nullptr)
		{
			std::fprintf(stderr, "TTF_OpenFont failed: %s\n", SDL_GetError());
			CloseFonts();
			TTF_Quit();
			return false;
		}

		TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
		g_fonts[scale] = font;
	}

	g_ttf_ready = true;
	return true;
#else
	std::fprintf(stderr,
	             "built without SDL3_ttf; fallback to built-in bitmap font\n");
	return false;
#endif
}

void Shutdown()
{
#if defined(THUNDER_HAVE_SDL3_TTF)
	CloseFonts();
	TTF_Quit();
#endif
}

bool UsingTrueType()
{
#if defined(THUNDER_HAVE_SDL3_TTF)
	return g_ttf_ready;
#else
	return false;
#endif
}

void DrawText(SDL_Renderer* renderer, int x, int y, int scale,
              const std::string& text, SDL_Color color)
{
#if defined(THUNDER_HAVE_SDL3_TTF)
	if(g_ttf_ready)
	{
		TTF_Font* font = g_fonts[ClampScale(scale)];
		SDL_Surface* surface =
		    TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
		if(surface != nullptr)
		{
			SDL_Texture* texture =
			    SDL_CreateTextureFromSurface(renderer, surface);
			if(texture != nullptr)
			{
				SDL_FRect dst{static_cast<float>(x),
				              static_cast<float>(y),
				              static_cast<float>(surface->w),
				              static_cast<float>(surface->h)};
				SDL_RenderTexture(renderer, texture, nullptr, &dst);
				SDL_DestroyTexture(texture);
			}
			SDL_DestroySurface(surface);
		}
		return;
	}
#endif
	BitmapFont::DrawText(renderer, x, y, scale, text, color);
}

int TextWidth(const std::string& text, int scale)
{
#if defined(THUNDER_HAVE_SDL3_TTF)
	if(g_ttf_ready)
	{
		int width = 0;
		int height = 0;
		if(TTF_GetStringSize(g_fonts[ClampScale(scale)], text.c_str(),
		                     text.size(), &width, &height))
			return width;
	}
#endif
	return BitmapFont::TextWidth(text, scale);
}

} // namespace SdlFont
