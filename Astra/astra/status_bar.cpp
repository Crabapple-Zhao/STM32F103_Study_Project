#include "status_bar.h"
#include "status_icons_generated.h"

namespace astra {
static StatusIconLayout &layout() {
  static StatusIconLayout instance(statusIconEntries, statusIconEntryCount);
  return instance;
}
bool statusBarSetVisible(const char *id, bool visible) { return layout().setVisible(id, visible); }
bool statusBarSetBitmap(const char *id, const Bitmap *bitmap) { return layout().setBitmap(id, bitmap); }
bool statusBarResetIcon(const char *id) { return layout().reset(id); }
void statusBarResetIcons() { layout().resetAll(); }
int statusBarDrawIcons(int width, int height, void (*pixel)(int, int)) {
  return layout().draw(width, height, pixel);
}
}
