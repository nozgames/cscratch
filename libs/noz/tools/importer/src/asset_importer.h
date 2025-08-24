//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct asset_importer_traits
{
	bool (*can_import) (path_t* path);
	void (*import_func) (path_t* source_path, path_t* output_dir, Props* config);
	bool (*does_depend_on) (path_t* source_path, path_t* dependency_path);

} asset_importer_traits_t;
