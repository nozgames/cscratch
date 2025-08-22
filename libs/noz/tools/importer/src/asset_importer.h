//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct asset_importer_traits
{
	bool (*can_import) (const path_t* path);
	void (*import_func) (const path_t* source_path, const path_t* output_dir);
	bool (*does_depend_on) (const path_t* source_path, const path_t* dependency_path);

} asset_importer_traits_t;
