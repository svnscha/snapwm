#include <windows.h>
#include "keyboard.h"
#include "tiling.h"
#include "error.h"
#include "config.h"

UINT getKeyCode(char value)
{
	SHORT keyCode = VkKeyScanEx(value, GetKeyboardLayout(0));

	if (keyCode == -1)
	{
		return 0;
	}

	return keyCode & 0xff;
}

bool addKeyboardKeybind(int id, char key, UINT modifiers)
{
	UINT keyCode = getKeyCode(key);
	BOOL didRegister = FALSE;
	DWORD lastError = ERROR_SUCCESS;

	if (keyCode != 0)
	{
		SetLastError(ERROR_SUCCESS);
		didRegister = RegisterHotKey(NULL, id, modifiers, keyCode);
		lastError = didRegister ? ERROR_SUCCESS : GetLastError();
	}

#ifdef DEBUG
	WCHAR message[256];
	wsprintfW(message, L"SnapWM hotkey register id=%d key=%C vk=0x%02x modifiers=0x%04x registered=%d error=%lu\n", id, key, keyCode, modifiers, didRegister, lastError);
	OutputDebugStringW(message);
#endif

	if (!didRegister)
	{
		reportWin32Error(L"Failed to register hotkey");
		return false;
	}

	return true;
}

bool initializeKeyboardConfig()
{
	bool didRegisterAll = true;

	didRegisterAll = addKeyboardKeybind(TOGGLE_FULLSCREEN_MODE_HOTKEY_ID, FULLSCREEN_MODE_HOTKEY, MOD_ALT | MOD_NOREPEAT) && didRegisterAll;
	didRegisterAll = addKeyboardKeybind(NEXT_WINDOW_HOTKEY_ID, NEXT_WINDOW_HOTKEY, MOD_ALT | MOD_NOREPEAT) && didRegisterAll;
	didRegisterAll = addKeyboardKeybind(PREV_WINDOW_HOTKEY_ID, PREV_WINDOW_HOTKEY, MOD_ALT | MOD_NOREPEAT) && didRegisterAll;
	didRegisterAll = addKeyboardKeybind(QUIT_HOTKEY_ID, QUIT_HOTKEY, MOD_ALT | MOD_NOREPEAT) && didRegisterAll;
	didRegisterAll = addKeyboardKeybind(FORCE_TILE_HOTKEY_ID, FORCE_TILE_HOTKEY, MOD_ALT | MOD_NOREPEAT) && didRegisterAll;
	didRegisterAll = addKeyboardKeybind(FORCE_TILE_MINIMIZED_HOTKEY_ID, FORCE_TILE_HOTKEY, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT) && didRegisterAll;
	didRegisterAll = addKeyboardKeybind(TOGGLE_DISABLE_ENABLE_TILING_HOTKEY_ID, TOGGLE_DISABLE_ENABLE_TILING_HOTKEY, MOD_ALT | MOD_NOREPEAT) && didRegisterAll;

	return didRegisterAll;
}

void cleanupKeyboard()
{
	UnregisterHotKey(NULL, TOGGLE_FULLSCREEN_MODE_HOTKEY_ID);
	UnregisterHotKey(NULL, NEXT_WINDOW_HOTKEY_ID);
	UnregisterHotKey(NULL, PREV_WINDOW_HOTKEY_ID);
	UnregisterHotKey(NULL, QUIT_HOTKEY_ID);
	UnregisterHotKey(NULL, FORCE_TILE_HOTKEY_ID);
	UnregisterHotKey(NULL, FORCE_TILE_MINIMIZED_HOTKEY_ID);
	UnregisterHotKey(NULL, TOGGLE_DISABLE_ENABLE_TILING_HOTKEY_ID);
}

void handleHotkey(WPARAM wparam, LPARAM lparam)
{
#ifdef DEBUG
	WCHAR message[128];
	wsprintfW(message, L"SnapWM hotkey dispatch id=%lu lparam=0x%p\n", wparam, lparam);
	OutputDebugStringW(message);
#endif

	switch (wparam)
	{
	case TOGGLE_FULLSCREEN_MODE_HOTKEY_ID:
		toggleFullscreenMode();
		break;
	case NEXT_WINDOW_HOTKEY_ID:
		focusNextWindow(false, 0);
		break;
	case PREV_WINDOW_HOTKEY_ID:
		focusNextWindow(true, 0);
		break;
	case FORCE_TILE_HOTKEY_ID:
		centerFocusedWindowAndTile();
		break;
	case FORCE_TILE_MINIMIZED_HOTKEY_ID:
		centerFocusedWindowAndTileIncludingMinimized();
		break;
	case TOGGLE_DISABLE_ENABLE_TILING_HOTKEY_ID:
		toggleDisableEnableTiling();
		break;
	}
}
