#include "tiling.h"
#include "error.h"
#include <Windows.h>
#include <dwmapi.h>

#define MAX_MANAGED 1024
#define FOCUSED_WINDOW_WIDTH_PERCENT 60
#define WINDOW_TEXT_BUFFER_LENGTH 256
#define DEBUG_LOG_BUFFER_LENGTH 1024
#define ERROR_OK 0

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#ifndef DWMWA_EXTENDED_FRAME_BOUNDS
#define DWMWA_EXTENDED_FRAME_BOUNDS 9
#endif

#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif

typedef struct
{
	HWND handle;
} ManagedWindow;

bool isFullscreen = false;
bool isTilingEnabled = true;
HWND managed[MAX_MANAGED];
ManagedWindow totalManaged[MAX_MANAGED];
HWND failedPlacementWindows[MAX_MANAGED];
HWND centeredWindow = NULL;
int numOfTotalManaged = 0;
int numOfCurrentlyManaged = 0;
int numOfFailedPlacements = 0;
int currentFocusedWindowIndex = 0;
bool shouldCenterFocusedWindow = true;
bool shouldIncludeMinimizedWindows = false;

ManagedWindow *searchManaged(HWND handle)
{
	for (int i = 0; i < numOfTotalManaged; i++)
	{
		if (totalManaged[i].handle == handle)
		{
			return &totalManaged[i];
		}
	}

	return NULL;
}

void cleanupManagedWindows()
{
	numOfTotalManaged = 0;
}

#ifdef DEBUG
void debugLogSkippedWindow(HWND windowHandle, WCHAR *reason)
{
	WCHAR className[WINDOW_TEXT_BUFFER_LENGTH];
	WCHAR caption[WINDOW_TEXT_BUFFER_LENGTH];
	WCHAR message[DEBUG_LOG_BUFFER_LENGTH];

	className[0] = L'\0';
	caption[0] = L'\0';

	GetClassNameW(windowHandle, className, WINDOW_TEXT_BUFFER_LENGTH);
	GetWindowTextW(windowHandle, caption, WINDOW_TEXT_BUFFER_LENGTH);

	wsprintfW(message, L"SnapWM skip reason=%s hwnd=%p class=\"%s\" caption=\"%s\" visible=%d iconic=%d\n", reason, windowHandle, className, caption, IsWindowVisible(windowHandle), IsIconic(windowHandle));
	OutputDebugStringW(message);
}
#endif

BOOL isWindowManagable(HWND windowHandle)
{
	WCHAR className[WINDOW_TEXT_BUFFER_LENGTH];
	WCHAR caption[WINDOW_TEXT_BUFFER_LENGTH];
	BOOL isCloaked = FALSE;

	if (!IsWindowVisible(windowHandle) || (!shouldIncludeMinimizedWindows && IsIconic(windowHandle)) || IsHungAppWindow(windowHandle))
	{
		return FALSE;
	}

	if (SUCCEEDED(DwmGetWindowAttribute(windowHandle, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked))) && isCloaked)
	{
#ifdef DEBUG
		debugLogSkippedWindow(windowHandle, L"cloaked");
#endif

		return FALSE;
	}

	className[0] = L'\0';
	caption[0] = L'\0';

	GetClassNameW(windowHandle, className, WINDOW_TEXT_BUFFER_LENGTH);
	GetWindowTextW(windowHandle, caption, WINDOW_TEXT_BUFFER_LENGTH);

	if (lstrcmpW(className, L"Windows.UI.Core.CoreWindow") == 0 && lstrcmpW(caption, L"Windows Input Experience") == 0)
	{
		return FALSE;
	}

	WINDOWINFO winInfo;
	winInfo.cbSize = sizeof(WINDOWINFO);
	if (!GetWindowInfo(windowHandle, &winInfo))
	{
		return FALSE;
	}

	if (winInfo.dwExStyle & WS_EX_TOOLWINDOW)
	{
#ifdef DEBUG
		debugLogSkippedWindow(windowHandle, L"tool-window");
#endif

		return FALSE;
	}

	if (GetWindow(windowHandle, GW_OWNER) != NULL && !(winInfo.dwExStyle & WS_EX_APPWINDOW))
	{
#ifdef DEBUG
		debugLogSkippedWindow(windowHandle, L"owned-helper-window");
#endif

		return FALSE;
	}

	if (winInfo.dwStyle & WS_POPUP)
	{
		return FALSE;
	}

	if (!(winInfo.dwExStyle & 0x20000000))
	{
		return FALSE;
	}

	if (GetWindowTextLengthW(windowHandle) == 0)
	{
		return FALSE;
	}

	RECT clientRect;
	if (!GetClientRect(windowHandle, &clientRect))
	{
		return FALSE;
	}

	// Skip small windows to avoid bugs
	if (clientRect.right < 100 || clientRect.bottom < 100)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CALLBACK windowEnumeratorCallback(HWND currentWindowHandle, LPARAM lparam)
{
	if (numOfTotalManaged > MAX_MANAGED)
	{
		return FALSE;
	}

	if (shouldIncludeMinimizedWindows && IsIconic(currentWindowHandle))
	{
		ShowWindow(currentWindowHandle, SW_RESTORE);
	}

	if (!isWindowManagable(currentWindowHandle))
	{
		return TRUE;
	}

	if (searchManaged(currentWindowHandle) != NULL)
	{
		return TRUE;
	}

	totalManaged[numOfTotalManaged].handle = currentWindowHandle;
	numOfTotalManaged++;

	return TRUE;
}

void updateManagedWindows()
{
	numOfCurrentlyManaged = 0;

	if (isFullscreen)
	{
		managed[0] = GetForegroundWindow();
		numOfCurrentlyManaged = 1;
		return;
	}

	for (int i = 0; i < numOfTotalManaged; i++)
	{
		if (!isWindowManagable(totalManaged[i].handle))
		{
			continue;
		}

		if (shouldIncludeMinimizedWindows && IsIconic(totalManaged[i].handle))
		{
			ShowWindow(totalManaged[i].handle, SW_RESTORE);
		}

		managed[numOfCurrentlyManaged] = totalManaged[i].handle;
		numOfCurrentlyManaged++;
	}
}

bool isFailedPlacementWindow(HWND windowHandle)
{
	for (int i = 0; i < numOfFailedPlacements; i++)
	{
		if (failedPlacementWindows[i] == windowHandle)
		{
			return true;
		}
	}

	return false;
}

void addFailedPlacementWindow(HWND windowHandle)
{
	if (numOfFailedPlacements >= MAX_MANAGED || isFailedPlacementWindow(windowHandle))
	{
		return;
	}

	failedPlacementWindows[numOfFailedPlacements] = windowHandle;
	numOfFailedPlacements++;
}

void resetFailedPlacementWindows()
{
	numOfFailedPlacements = 0;
}

void cleanupFailedPlacementWindows()
{
	int keepCounter = 0;

	for (int i = 0; i < numOfCurrentlyManaged; i++)
	{
		if (isFailedPlacementWindow(managed[i]))
		{
			continue;
		}

		managed[keepCounter] = managed[i];
		keepCounter++;
	}

	numOfCurrentlyManaged = keepCounter;
}

#ifdef DEBUG
void debugLogWindowPlacement(HWND windowHandle, WCHAR *slot, int x, int y, int width, int height, BOOL didMove, DWORD lastError)
{
	WCHAR className[WINDOW_TEXT_BUFFER_LENGTH];
	WCHAR caption[WINDOW_TEXT_BUFFER_LENGTH];
	WCHAR message[DEBUG_LOG_BUFFER_LENGTH];
	RECT actualRect;
	int actualX = 0;
	int actualY = 0;
	int actualWidth = 0;
	int actualHeight = 0;

	className[0] = L'\0';
	caption[0] = L'\0';

	GetClassNameW(windowHandle, className, WINDOW_TEXT_BUFFER_LENGTH);
	GetWindowTextW(windowHandle, caption, WINDOW_TEXT_BUFFER_LENGTH);

	if (FAILED(DwmGetWindowAttribute(windowHandle, DWMWA_EXTENDED_FRAME_BOUNDS, &actualRect, sizeof(actualRect))) && !GetWindowRect(windowHandle, &actualRect))
	{
		actualRect.left = 0;
		actualRect.top = 0;
		actualRect.right = 0;
		actualRect.bottom = 0;
	}
	else
	{
		actualX = actualRect.left;
		actualY = actualRect.top;
		actualWidth = actualRect.right - actualRect.left;
		actualHeight = actualRect.bottom - actualRect.top;
	}

	wsprintfW(message, L"SnapWM tile slot=%s hwnd=%p class=\"%s\" caption=\"%s\" wantedFrame=(%d,%d %dx%d) actualFrame=(%d,%d %dx%d) moved=%d error=%lu visible=%d iconic=%d\n", slot, windowHandle, className, caption, x, y, width, height, actualX, actualY, actualWidth, actualHeight, didMove, lastError, IsWindowVisible(windowHandle), IsIconic(windowHandle));
	OutputDebugStringW(message);
}
#endif

void getWindowRectForVisibleFrame(HWND windowHandle, int x, int y, int width, int height, RECT *windowRect)
{
	RECT currentWindowRect;
	RECT currentFrameRect;

	windowRect->left = x;
	windowRect->top = y;
	windowRect->right = x + width;
	windowRect->bottom = y + height;

	if (!GetWindowRect(windowHandle, &currentWindowRect))
	{
		return;
	}

	if (FAILED(DwmGetWindowAttribute(windowHandle, DWMWA_EXTENDED_FRAME_BOUNDS, &currentFrameRect, sizeof(currentFrameRect))))
	{
		return;
	}

	int leftInset = currentFrameRect.left - currentWindowRect.left;
	int topInset = currentFrameRect.top - currentWindowRect.top;
	int rightInset = currentWindowRect.right - currentFrameRect.right;
	int bottomInset = currentWindowRect.bottom - currentFrameRect.bottom;

	windowRect->left = x - leftInset;
	windowRect->top = y - topInset;
	windowRect->right = x + width + rightInset;
	windowRect->bottom = y + height + bottomInset;
}

BOOL applyWindowPosition(HWND windowHandle, WCHAR *slot, int x, int y, int width, int height)
{
	SetLastError(ERROR_OK);
	BOOL didMove = SetWindowPos(windowHandle, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
	DWORD lastError = didMove ? ERROR_OK : GetLastError();

	if (!didMove)
	{
		addFailedPlacementWindow(windowHandle);
	}

#ifdef DEBUG
	debugLogWindowPlacement(windowHandle, slot, x, y, width, height, didMove, lastError);
#endif

	return didMove;
}

BOOL applyVisibleFramePosition(HWND windowHandle, WCHAR *slot, int x, int y, int width, int height)
{
	RECT windowRect;
	getWindowRectForVisibleFrame(windowHandle, x, y, width, height, &windowRect);

	int windowX = windowRect.left;
	int windowY = windowRect.top;
	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	if (windowWidth < 1)
	{
		windowWidth = 1;
	}

	if (windowHeight < 1)
	{
		windowHeight = 1;
	}

	SetLastError(ERROR_OK);
	BOOL didMove = SetWindowPos(windowHandle, NULL, windowX, windowY, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	DWORD lastError = didMove ? ERROR_OK : GetLastError();

	if (!didMove)
	{
		addFailedPlacementWindow(windowHandle);
	}

#ifdef DEBUG
	debugLogWindowPlacement(windowHandle, slot, x, y, width, height, didMove, lastError);
#endif

	return didMove;
}

void styleTiledWindow(HWND windowHandle)
{
	COLORREF borderColor = RGB(0, 0, 0);
	int cornerPreference = 1;

	DwmSetWindowAttribute(windowHandle, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
	DwmSetWindowAttribute(windowHandle, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
}

BOOL positionWindow(HWND windowHandle, WCHAR *slot, int x, int y, int width, int height)
{
	styleTiledWindow(windowHandle);

	return applyVisibleFramePosition(windowHandle, slot, x, y, width, height);
}

BOOL positionWindowWithoutMargins(HWND windowHandle, WCHAR *slot, int x, int y, int width, int height)
{
	return applyWindowPosition(windowHandle, slot, x, y, width, height);
}

BOOL positionSideWindows(WCHAR *slot, int firstWindowIndex, int windowCount, int x, int y, int width, int height)
{
	BOOL didMoveAll = TRUE;

	if (windowCount == 0)
	{
		return TRUE;
	}

	int windowHeight = height / windowCount;

	for (int i = 0; i < windowCount; i++)
	{
		int currentY = y + (windowHeight * i);
		int currentHeight = i == windowCount - 1 ? height - (windowHeight * i) : windowHeight;

		if (!positionWindow(managed[firstWindowIndex + i], slot, x, currentY, width, currentHeight))
		{
			didMoveAll = FALSE;
		}
	}

	return didMoveAll;
}

bool moveWindowToCenter(HWND windowHandle)
{
	for (int i = 0; i < numOfCurrentlyManaged; i++)
	{
		if (managed[i] != windowHandle)
		{
			continue;
		}

		HWND managedWindow = managed[i];

		for (int j = i; j > 0; j--)
		{
			managed[j] = managed[j - 1];
		}

		managed[0] = managedWindow;
		currentFocusedWindowIndex = 0;
		return true;
	}

	return false;
}

void updateCenteredWindow()
{
	if (shouldCenterFocusedWindow)
	{
		centeredWindow = GetForegroundWindow();
		shouldCenterFocusedWindow = false;
	}

	if (moveWindowToCenter(centeredWindow))
	{
		return;
	}

	centeredWindow = managed[0];
	moveWindowToCenter(centeredWindow);
}

void tileManagedWindows()
{
	if (numOfCurrentlyManaged == 0)
	{
		return;
	}

	while (numOfCurrentlyManaged > 0)
	{
		resetFailedPlacementWindows();
		updateCenteredWindow();

		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);

		HMONITOR monitorHandle = MonitorFromWindow(managed[0], MONITOR_DEFAULTTONEAREST);

		if (!GetMonitorInfoW(monitorHandle, &monitorInfo))
		{
			return;
		}

		RECT workArea = monitorInfo.rcWork;
		int screenWidth = workArea.right - workArea.left;
		int screenHeight = workArea.bottom - workArea.top;

		if (isFullscreen)
		{
			positionWindowWithoutMargins(managed[0], L"fullscreen", workArea.left, workArea.top, screenWidth, screenHeight);
		}
		else
		{
			int focusedWindowWidth = (screenWidth * FOCUSED_WINDOW_WIDTH_PERCENT) / 100;
			int sideWindowWidth = (screenWidth - focusedWindowWidth) / 2;
			int rightWindowWidth = screenWidth - focusedWindowWidth - sideWindowWidth;
			int remainingWindowCount = numOfCurrentlyManaged - 1;
			int leftWindowCount = (remainingWindowCount + 1) / 2;
			int rightWindowCount = remainingWindowCount - leftWindowCount;

			positionWindow(managed[0], L"center", workArea.left + sideWindowWidth, workArea.top, focusedWindowWidth, screenHeight);
			positionSideWindows(L"left", 1, leftWindowCount, workArea.left, workArea.top, sideWindowWidth, screenHeight);
			positionSideWindows(L"right", 1 + leftWindowCount, rightWindowCount, workArea.left + sideWindowWidth + focusedWindowWidth, workArea.top, rightWindowWidth, screenHeight);
		}

		if (numOfFailedPlacements == 0)
		{
			return;
		}

		cleanupFailedPlacementWindows();
	}
}

void tileWindows()
{
	if (!isTilingEnabled)
	{
		return;
	}

	if (!isFullscreen)
	{
		cleanupManagedWindows();
	}

	EnumWindows(windowEnumeratorCallback, 0);

	if (numOfTotalManaged == 0)
	{
		return;
	}

	updateManagedWindows();

	tileManagedWindows();
}

void centerFocusedWindowAndTile()
{
	shouldCenterFocusedWindow = true;
	tileWindows();
}

void centerFocusedWindowAndTileIncludingMinimized()
{
	shouldCenterFocusedWindow = true;
	shouldIncludeMinimizedWindows = true;
	tileWindows();
	shouldIncludeMinimizedWindows = false;
}

void toggleFullscreenMode()
{
	isFullscreen = !isFullscreen;
	tileWindows();
}

void focusNextWindow(bool goBack, unsigned int callCount)
{
	// Avoid infinite recursion
	if (callCount > 25)
	{
		tileWindows();
		return;
	}

	if (isFullscreen)
	{
		toggleFullscreenMode();
	}

	currentFocusedWindowIndex += goBack ? -1 : 1;

	if (currentFocusedWindowIndex < 0)
	{
		currentFocusedWindowIndex = numOfCurrentlyManaged - 1;
	}
	else if (currentFocusedWindowIndex >= numOfCurrentlyManaged)
	{
		currentFocusedWindowIndex = 0;
	}

	if (!isWindowManagable(managed[currentFocusedWindowIndex]) || (GetForegroundWindow() == managed[currentFocusedWindowIndex]))
	{
		focusNextWindow(goBack, ++callCount);
	}

	SwitchToThisWindow(managed[currentFocusedWindowIndex], FALSE);
}

void toggleDisableEnableTiling()
{
	isTilingEnabled = !isTilingEnabled;

	if (isTilingEnabled)
	{
		tileWindows();
	}
}
