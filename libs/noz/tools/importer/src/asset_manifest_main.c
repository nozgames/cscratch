//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "asset_manifest.h"
#include <stdio.h>

// Command line interface for the asset manifest generator
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <assets_directory> <output_manifest.c>\n", argv[0]);
        printf("  assets_directory: Path to the imported assets directory\n");
        printf("  output_manifest.c: Path where to generate the manifest C file\n");
        return 1;
    }

    const char* assets_dir = argv[1];
    const char* output_file = argv[2];

    printf("NoZ Asset Manifest Generator\n");
    printf("============================\n");

    if (!asset_manifest_generate(assets_dir, output_file))
    {
        printf("Failed to generate asset manifest\n");
        return 1;
    }

    printf("Asset manifest generation completed successfully\n");
    return 0;
}