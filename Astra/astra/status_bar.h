#pragma once
#include "astra_icon.h"

namespace astra {
// Call from the UI/main loop, not interrupts. IDs are defined in status_icons.json.
// Overrides borrow immutable Bitmap/data; both must outlive use (prefer static const).
bool statusBarSetVisible(const char *id, bool visible);
bool statusBarSetBitmap(const char *id, const Bitmap *bitmap);
bool statusBarResetIcon(const char *id);
void statusBarResetIcons();

// Draws within the top bar; reserves title space and returns its right boundary.
int statusBarDrawIcons(int width, int height, void (*pixel)(int, int));
}
