#include "io/score_path.h"

#include <filesystem>

namespace thunder
{

std::string ResolveScorePath(const std::string& executable_dir)
{
	const std::filesystem::path dir(executable_dir);
	const std::filesystem::path local = dir / "highscore.txt";

	if(std::filesystem::exists(local))
		return local.string();

	const std::filesystem::path legacy = dir / ".." / "highscore.txt";
	if(std::filesystem::exists(legacy))
		return legacy.string();

	return local.string();
}

} // namespace thunder
