//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <filesystem>
#include <string>

struct AssetImporterTraits
{
	bool (*can_import) (const std::filesystem::path& path);
	void (*import_func) (const std::filesystem::path& source_path, const std::filesystem::path& output_dir, Props* config);
	bool (*does_depend_on) (const std::filesystem::path& source_path, const std::filesystem::path& dependency_path);
};
