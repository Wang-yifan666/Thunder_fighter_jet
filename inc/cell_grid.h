#pragma once

#include <string>
#include <vector>

#include "game/world_snapshot.h"

namespace CellGrid
{

/// 把世界快照画进字符缓冲区（终端展示逻辑，隶属终端层）。
/// 字形、坐标、PAUSED 覆盖方式与重构前 def.cpp 中的实现逐字节一致。
std::vector<std::string> Build(const thunder::WorldSnapshot& snapshot,
                               bool paused, int width, int height);

} // namespace CellGrid
