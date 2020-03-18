#include "resource.h"
#include "stdafx.h"
#include "Engine.h"
#include <comdef.h>
#include <WinUser.h>
#include <windowsx.h>

const UINT g_width = 800;
const UINT g_height = 600;
Engine g_engine(g_width, g_height);

LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	const WCHAR * WND_CLASS_NAME = TEXT("MyWndClassName");

	WNDCLASSEX wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpszClassName = WND_CLASS_NAME;
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = wndProc;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);

	ATOM hr = RegisterClassEx(&wndClass);
	if (hr == NULL)
	{
		exit(-1);
	}

	RECT windowRect;
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.right = g_width;
	windowRect.bottom = g_height;

	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindowEx(
		0,
		WND_CLASS_NAME,
		TEXT("DirectX 12 Normal Mapping"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,	// x
		CW_USEDEFAULT,	// y
		g_width,	// width
		g_height,	// height
		nullptr,
		nullptr,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		exit(-1);
	}

	ShowWindow(hwnd, nCmdShow);

	g_engine.Init(hwnd);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	g_engine.Destroy();
	return 0;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		g_engine.Update();
		g_engine.Render();
		return 0;
	case WM_MOUSEMOVE:
		{
			const WPARAM rightMouseButtonFlag = 0x0002;
			bool rightMouseBtnIsDown = (wParam & rightMouseButtonFlag);
			g_engine.Input(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), rightMouseBtnIsDown);
			return 0;
		}
	case WM_SIZE:
		g_engine.ResizeViewport(LOWORD(lParam), HIWORD(lParam));
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
