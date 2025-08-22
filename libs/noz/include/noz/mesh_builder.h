#pragma once

typedef struct mesh_builder* mesh_builder_t;

mesh_builder_t mesh_builder_create(int max_vertices, int max_indices);