//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct asset_importer_traits
{
	bool (*can_import) (Path* path);
	void (*import_func) (Path* source_path, Path* output_dir, Props* config);
	bool (*does_depend_on) (Path* source_path, Path* dependency_path);

} asset_importer_traits_t;
