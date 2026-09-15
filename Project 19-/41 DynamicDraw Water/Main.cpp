#include "GameApp.h"
#include <filesystem>
#include <fstream>
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR command, int)
{
    const bool validation = std::string(command).find("--validate") != std::string::npos;
    wchar_t path[32768];
    GetModuleFileNameW(nullptr, path, 32768);
    std::filesystem::current_path(std::filesystem::path(path).parent_path());
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    int result = 1;
    try
    {
        GameApp app(instance, validation);
        if (app.Init())
            result = validation ? app.Validate() : app.Run();
    }
    catch (const std::exception &e)
    {
        std::ofstream("error.log") << e.what();
        if (!validation)
            MessageBoxA(nullptr, e.what(), "Water sample error", MB_ICONERROR);
    }
    if (SUCCEEDED(com))
        CoUninitialize();
    return result;
}
