/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#ifdef _WIN32

#include <windows.h>

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
