//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

enum
{
	// all noz engine types are negative and user types are positive
    type_unknown = -1000,

    // @object
    type_stream,
    type_list,
    type_map,
    type_props,
    type_file_watcher,
    type_mesh_builder,

    // @asset
    type_texture,
    type_material,
    type_shader,
    type_font,
    type_mesh,
    type_sound,

    // @scene
    type_entity,
    type_camera
};
