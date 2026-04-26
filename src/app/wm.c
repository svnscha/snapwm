#include <windows.h>
#include <shellapi.h>
#include "tiling.h"
#include "error.h"
#include "config.h"
#include "keyboard.h"
#include "messages.h"
#include "shared_mem.h"

#define EXIT_OK 0
#define EXIT_FAILED 1
#define LWM_TRAY_EVENT (WM_APP + 1)
#define TRAY_ICON_ID 1
#define TRAY_COMMAND_TILE 100
#define TRAY_COMMAND_TILE_MINIMIZED 101
#define TRAY_COMMAND_NEXT_WINDOW 102
#define TRAY_COMMAND_PREVIOUS_WINDOW 103
#define TRAY_COMMAND_FULLSCREEN 104
#define TRAY_COMMAND_TOGGLE_TILING 105
#define TRAY_COMMAND_EXIT 106

HICON trayIcon;
HWND trayWindow;

void clearMemory(void *memory, int size)
{
	BYTE *bytes = memory;

	for (int i = 0; i < size; i++)
	{
		bytes[i] = 0;
	}
}

HICON createTileIcon(HINSTANCE instance)
{
	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	BITMAPINFO bitmapInfo;
	HDC screenDc = GetDC(NULL);
	HDC iconDc = CreateCompatibleDC(screenDc);
	HBITMAP colorBitmap;
	HBITMAP maskBitmap;
	HBITMAP oldBitmap;
	HBRUSH backgroundBrush = CreateSolidBrush(RGB(8, 8, 8));
	HBRUSH centerBrush = CreateSolidBrush(RGB(245, 245, 245));
	HBRUSH sideBrush = CreateSolidBrush(RGB(60, 180, 200));
	RECT rect;
	BYTE maskBits[512];
	ICONINFO iconInfo;
	HICON icon;
	void *bits;
	int sideWidth = iconWidth / 5;
	int centerWidth = iconWidth - (sideWidth * 2) - 4;

	clearMemory(&bitmapInfo, sizeof(bitmapInfo));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = iconWidth;
	bitmapInfo.bmiHeader.biHeight = -iconHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	colorBitmap = CreateDIBSection(screenDc, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
	oldBitmap = SelectObject(iconDc, colorBitmap);

	rect.left = 0;
	rect.top = 0;
	rect.right = iconWidth;
	rect.bottom = iconHeight;
	FillRect(iconDc, &rect, backgroundBrush);

	rect.left = 2;
	rect.top = 2;
	rect.right = 2 + sideWidth;
	rect.bottom = iconHeight - 2;
	FillRect(iconDc, &rect, sideBrush);

	rect.left = iconWidth - sideWidth - 2;
	rect.right = iconWidth - 2;
	FillRect(iconDc, &rect, sideBrush);

	rect.left = sideWidth + 3;
	rect.top = 2;
	rect.right = rect.left + centerWidth;
	rect.bottom = iconHeight - 2;
	FillRect(iconDc, &rect, centerBrush);

	clearMemory(maskBits, sizeof(maskBits));
	maskBitmap = CreateBitmap(iconWidth, iconHeight, 1, 1, maskBits);

	iconInfo.fIcon = TRUE;
	iconInfo.xHotspot = 0;
	iconInfo.yHotspot = 0;
	iconInfo.hbmMask = maskBitmap;
	iconInfo.hbmColor = colorBitmap;
	icon = CreateIconIndirect(&iconInfo);

	SelectObject(iconDc, oldBitmap);
	DeleteObject(backgroundBrush);
	DeleteObject(centerBrush);
	DeleteObject(sideBrush);
	DeleteObject(colorBitmap);
	DeleteObject(maskBitmap);
	DeleteDC(iconDc);
	ReleaseDC(NULL, screenDc);

	return icon;
}

void showTrayMenu(HWND windowHandle)
{
	HMENU menu = CreatePopupMenu();
	POINT cursorPosition;

	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_TILE, L"&Tile\tAlt+T");
	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_TILE_MINIMIZED, L"Tile &Minimized\tAlt+Shift+T");
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_NEXT_WINDOW, L"&Next Window\tAlt+J");
	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_PREVIOUS_WINDOW, L"&Previous Window\tAlt+K");
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_FULLSCREEN, L"Toggle &Fullscreen\tAlt+F");
	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_TOGGLE_TILING, L"Toggle T&iling\tAlt+P");
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(menu, MF_STRING, TRAY_COMMAND_EXIT, L"E&xit\tAlt+Q");

	GetCursorPos(&cursorPosition);
	SetForegroundWindow(windowHandle);
	TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursorPosition.x, cursorPosition.y, 0, windowHandle, NULL);
	DestroyMenu(menu);
}

void handleTrayCommand(HWND windowHandle, WPARAM command)
{
	switch (LOWORD(command))
	{
	case TRAY_COMMAND_TILE:
		centerFocusedWindowAndTile();
		break;
	case TRAY_COMMAND_TILE_MINIMIZED:
		centerFocusedWindowAndTileIncludingMinimized();
		break;
	case TRAY_COMMAND_NEXT_WINDOW:
		focusNextWindow(false, 0);
		break;
	case TRAY_COMMAND_PREVIOUS_WINDOW:
		focusNextWindow(true, 0);
		break;
	case TRAY_COMMAND_FULLSCREEN:
		toggleFullscreenMode();
		break;
	case TRAY_COMMAND_TOGGLE_TILING:
		toggleDisableEnableTiling();
		break;
	case TRAY_COMMAND_EXIT:
		PostMessageW(windowHandle, WM_CLOSE, 0, 0);
		break;
	}
}

LRESULT CALLBACK trayWindowProc(HWND windowHandle, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case LWM_TRAY_EVENT:
		if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU)
		{
			showTrayMenu(windowHandle);
		}
		else if (lparam == WM_LBUTTONDBLCLK)
		{
			centerFocusedWindowAndTile();
		}
		return 0;
	case WM_COMMAND:
		handleTrayCommand(windowHandle, wparam);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(EXIT_OK);
		return 0;
	}

	return DefWindowProcW(windowHandle, message, wparam, lparam);
}

HWND createTrayWindow(HINSTANCE instance)
{
	WNDCLASSW windowClass;

	clearMemory(&windowClass, sizeof(windowClass));
	windowClass.lpfnWndProc = trayWindowProc;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"SnapWMTrayWindow";

	if (!RegisterClassW(&windowClass))
	{
		return NULL;
	}

	return CreateWindowExW(0, windowClass.lpszClassName, L"SnapWM", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, instance, NULL);
}

BOOL addTrayIcon(HWND windowHandle, HICON icon)
{
	NOTIFYICONDATAW iconData;

	clearMemory(&iconData, sizeof(iconData));
	iconData.cbSize = sizeof(iconData);
	iconData.hWnd = windowHandle;
	iconData.uID = TRAY_ICON_ID;
	iconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	iconData.uCallbackMessage = LWM_TRAY_EVENT;
	iconData.hIcon = icon;
	lstrcpyW(iconData.szTip, L"SnapWM");

	return Shell_NotifyIconW(NIM_ADD, &iconData);
}

void removeTrayIcon(HWND windowHandle)
{
	NOTIFYICONDATAW iconData;

	clearMemory(&iconData, sizeof(iconData));
	iconData.cbSize = sizeof(iconData);
	iconData.hWnd = windowHandle;
	iconData.uID = TRAY_ICON_ID;

	Shell_NotifyIconW(NIM_DELETE, &iconData);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmdLine, int cmdShow)
{
	int exitCode = EXIT_FAILED;

	HMODULE wmDll = NULL;
	HHOOK hookShellProcHandle = NULL;

	HANDLE currentlyRunningMutex = CreateMutexW(NULL, TRUE, L"Global\\SnapWMIsCurrentlyRunning");

	if (currentlyRunningMutex == NULL)
	{
		reportWin32Error(L"Failed creating currently running mutex");
		goto cleanup;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		reportGeneralError(L"SnapWM is already running, exiting");
		goto cleanup;
	}

	SetProcessDPIAware();

	trayWindow = createTrayWindow(instance);
	if (trayWindow == NULL)
	{
		reportWin32Error(L"Failed creating tray window");
		goto cleanup;
	}

	trayIcon = createTileIcon(instance);
	if (trayIcon == NULL)
	{
		reportWin32Error(L"Failed creating tray icon");
		goto cleanup;
	}

	if (!addTrayIcon(trayWindow, trayIcon))
	{
		reportWin32Error(L"Failed adding tray icon");
		goto cleanup;
	}

	if (!initializeKeyboardConfig())
	{
		reportGeneralError(L"Setup keyboard config");
		goto cleanup;
	}

	if (!storeDwordInSharedMemory(GetCurrentThreadId()))
	{
		reportGeneralError(L"Failed writing thread id to shared memory");
		goto cleanup;
	}

	wmDll = LoadLibraryW(L"snapwm");

	if (wmDll == NULL)
	{
		reportWin32Error(L"LoadLibrary of snapwm");
		goto cleanup;
	}

	FARPROC shellProc = GetProcAddress(wmDll, "ShellProc");

	if (shellProc == NULL)
	{
		reportWin32Error(L"GetProcAddress for ShellProc");
		goto cleanup;
	}

	hookShellProcHandle = SetWindowsHookExW(WH_SHELL, (HOOKPROC)shellProc, wmDll, 0);

	if (hookShellProcHandle == NULL)
	{
		reportWin32Error(L"SetWindowsHookExW for shell hook");
		goto cleanup;
	}

	tileWindows();

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		switch (msg.message)
		{
		case WM_HOTKEY:
#ifdef DEBUG
			WCHAR message[128];
			wsprintfW(message, L"SnapWM hotkey message id=%lu lparam=0x%p\n", msg.wParam, msg.lParam);
			OutputDebugStringW(message);
#endif

			if (msg.wParam == QUIT_HOTKEY_ID)
			{
				exitCode = EXIT_OK;
				goto cleanup;
			}

			handleHotkey(msg.wParam, msg.lParam);
			break;
		case LWM_WINDOW_EVENT:
			tileWindows();
			break;
		}
	}

	exitCode = (int)msg.wParam;

cleanup:
	if (trayWindow != NULL)
	{
		removeTrayIcon(trayWindow);
		DestroyWindow(trayWindow);
	}

	if (trayIcon != NULL)
	{
		DestroyIcon(trayIcon);
	}

	if (currentlyRunningMutex != NULL)
	{
		CloseHandle(currentlyRunningMutex);
	}

	cleanupKeyboard();

	if (hookShellProcHandle)
	{
		UnhookWindowsHookEx(hookShellProcHandle);
	}

	if (wmDll)
	{
		FreeLibrary(wmDll);
	}

	cleanupMemoryMapFile();

	return exitCode;
}
