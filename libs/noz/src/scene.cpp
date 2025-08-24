//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void InitEntity();

void InitScene()
{
    InitEntity();
    InitCamera();
}

void ShutdownScene()
{
    ShutdownCamera();
}
