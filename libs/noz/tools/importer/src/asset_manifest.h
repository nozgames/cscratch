//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <noz/noz.h>

// Asset manifest generator function
// Scans the given output directory for asset files and generates a C manifest file
// containing asset metadata and memory requirements
//
// @param output_directory: Path to the directory containing imported assets
// @param manifest_output_path: Path where to generate the manifest C file
// @return: true on success, false on failure
bool asset_manifest_generate(const char* output_directory, const char* manifest_output_path);