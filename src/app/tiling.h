#pragma once

#include <Windows.h>
#include <stdbool.h>

void tileWindows();
void centerFocusedWindowAndTile();
void centerFocusedWindowAndTileIncludingMinimized();
void toggleFullscreenMode();
void focusNextWindow(bool, unsigned int);
void toggleDisableEnableTiling();
