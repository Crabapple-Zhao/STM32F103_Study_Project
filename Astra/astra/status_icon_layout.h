#pragma once
#include "astra_icon.h"
#include <stddef.h>
#include <string.h>

namespace astra {
struct StatusIconEntry {
  const char *id;
  const Bitmap *defaultBitmap;
  bool defaultVisible;
  const Bitmap *bitmap;
  bool visible;
};

// Fixed storage, no heap allocation. List order is priority from right to left.
class StatusIconLayout {
 public:
  StatusIconLayout(StatusIconEntry *entries, size_t count) : entries_(entries), count_(count) {}
  bool setVisible(const char *id, bool visible) {
    StatusIconEntry *entry = find(id);
    if (!entry) return false;
    entry->visible = visible;
    return true;
  }
  bool setBitmap(const char *id, const Bitmap *bitmap) {
    StatusIconEntry *entry = find(id);
    if (!entry || !bitmap || !bitmap->valid()) return false;
    entry->bitmap = bitmap;
    return true;
  }
  bool reset(const char *id) {
    StatusIconEntry *entry = find(id);
    if (!entry) return false;
    restore(*entry);
    return true;
  }
  void resetAll() {
    for (size_t i = 0; i < count_; ++i) restore(entries_[i]);
  }
  int draw(int width, int height, void (*pixel)(int, int), int gap = 2, int titleWidth = 32) const {
    if (width <= 0 || height <= 1 || !pixel || gap < 0 || titleWidth < 0) return 0;
    int right = width - gap;
    bool drawn = false;
    for (size_t i = 0; i < count_; ++i) {
      const StatusIconEntry &entry = entries_[i];
      const Bitmap *bmp = entry.bitmap;
      if (!entry.visible || !bmp || !bmp->valid() || bmp->height > height - 1) continue;
      int x = right - bmp->width;
      if (x < 0 || x - 2 * gap < titleWidth) continue;
      int y = (height - bmp->height) / 2;
      int stride = (bmp->width + 7) / 8;
      for (int row = 0; row < bmp->height; ++row) {
        for (int col = 0; col < bmp->width; ++col) {
          if (bmp->data[row * stride + col / 8] & (1 << (col % 8))) pixel(x + col, y + row);
        }
      }
      right = x - gap;
      drawn = true;
    }
    return drawn ? right - gap : width;
  }
 private:
  StatusIconEntry *entries_;
  size_t count_;
  StatusIconEntry *find(const char *id) {
    if (!id) return nullptr;
    for (size_t i = 0; i < count_; ++i)
      if (strcmp(entries_[i].id, id) == 0) return &entries_[i];
    return nullptr;
  }
  static void restore(StatusIconEntry &entry) {
    entry.bitmap = entry.defaultBitmap;
    entry.visible = entry.defaultVisible;
  }
};
}
