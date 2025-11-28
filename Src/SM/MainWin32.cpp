#include "SM/Engine.h"
#include "SM/Assert.h"

#include "SM/PlatformWin32.cpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>

using namespace sm;

LRESULT EngineWinProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
        }
        break;

        case WM_DESTROY:
        {
        }
        break;

        case WM_CLOSE:
        {
        }
        break;

        case WM_ACTIVATEAPP:
        {
        }
        break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;
    }

    return result;
}

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    using namespace sm;

    HMODULE workbenchDLL = ::LoadLibraryA("Workbench.dll");
    SM_ASSERT(workbenchDLL != NULL);
    if(workbenchDLL == NULL)
    {
        Platform::Log("Not a valid workbench dll\n");
        return 1;
    }

    GameLoadedFunction GameLoaded = (GameLoadedFunction)::GetProcAddress(workbenchDLL, "GameLoaded");
    if(GameLoaded == NULL)
    {
        Platform::Log("Couldn't find GameLoaded in DLL\n");
        return 1;
    }

    GameUpdateFunction GameUpdate = (GameUpdateFunction)::GetProcAddress(workbenchDLL, "GameUpdate");
    if(GameUpdate == NULL)
    {
        Platform::Log("Couldn't find GameUpdate in DLL\n");
        return 1;
    }

    GameRenderFunction GameRender = (GameRenderFunction)::GetProcAddress(workbenchDLL, "GameRender");
    if(GameRender == NULL)
    {
        Platform::Log("Couldn't find GameRender in DLL\n");
        return 1;
    }

    sm::EngineApi engineApi = {
        .EngineLog = &Platform::Log
    };
    GameLoaded(engineApi);

    GameUpdate();
    GameRender();

    //WNDCLASS windowClass = {};
    //windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    //windowClass.lpfnWndProc = EngineWinProc;
    //int         cbClsExtra;
    //int         cbWndExtra;
    //HINSTANCE   hInstance;
    //HICON       hIcon;
    //HCURSOR     hCursor;
    //HBRUSH      hbrBackground;
    //LPCSTR      lpszMenuName;
    //LPCSTR      lpszClassName;
    return 0;
}
