//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef _WIN32

NOZ_WARNINGS_DISABLE()

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

extern int main(int argc, char* argv[]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Convert command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;
   
    char* args[1] = {".exe"};
    return main(1, args);
}

#endif

NOZ_WARNINGS_ENABLE()