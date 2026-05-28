#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>

namespace RenderConsole
{

/// Convert a game screen character to a styled FTXUI element.
/// @param c       the character from the screen buffer
/// @param inv     whether the player is currently invincible
/// @param paused  whether the game is paused
/// @param y       row index in the screen buffer
/// @param height  total screen height (rows)
ftxui::Element CellToElement(char c, bool inv, bool paused, int y,
                             int height);

/// Build the life-display string (e.g. "剩余生命: <> <> <>  ++ ")
/// @param life    current life count
/// @param max_show  cap at which " ++ " is appended
std::string LifeString(int life, int max_show = 10);

} // namespace RenderConsole