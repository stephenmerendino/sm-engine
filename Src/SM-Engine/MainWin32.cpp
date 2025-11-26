#define WIN32_LEAN_AND_MEAN
#include <windows.h>

LRESULT EngineWinProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
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
        }
        break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = EngineWinProc;
    int         cbClsExtra;
    int         cbWndExtra;
    HINSTANCE   hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
    return 0;
}
