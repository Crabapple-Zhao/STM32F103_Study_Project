#include "../../Astra/astra/status_bar.h"
#include "../../Astra/astra/status_icons_generated.h"
#include <cassert>
#include <cstring>
#include <cstdio>

static bool pixels[20][128];
static void pixel(int x, int y) {
  assert(x >= 0 && x < 128 && y >= 0 && y < 19);
  pixels[y][x] = true;
}
static void clear() { memset(pixels, 0, sizeof(pixels)); }
static void expectedBitmap(bool expected[20][128], const astra::Bitmap &bmp, int x) {
  for (int y = 0; y < bmp.height; ++y)
    for (int col = 0; col < bmp.width; ++col)
      if (bmp.data[y*((bmp.width+7)/8)+col/8] & (1<<(col%8)))
        expected[(20-bmp.height)/2+y][x+col] = true;
}
int main() {
  using namespace astra;
  assert(statusIconEntryCount == 3 && statusIconEntries[0].visible);
  bool expected[20][128] = {};
  expectedBitmap(expected, status_bitmap_battery, 110);
  expectedBitmap(expected, status_bitmap_wifi, 94);
  expectedBitmap(expected, status_bitmap_tf, 78);
  assert(statusBarDrawIcons(128,20,pixel)==74);
  assert(memcmp(pixels,expected,sizeof(pixels))==0);
  assert(statusBarSetVisible("wifi",false));
  clear(); memset(expected,0,sizeof(expected));
  expectedBitmap(expected,status_bitmap_battery,110);
  expectedBitmap(expected,status_bitmap_tf,94);
  assert(statusBarDrawIcons(128,20,pixel)==90);
  assert(memcmp(pixels,expected,sizeof(pixels))==0);
  assert(!statusBarSetVisible("missing",true));
  assert(!statusBarSetVisible(nullptr,true));
  assert(!statusBarSetBitmap("wifi",nullptr));
  const uint8_t data[64] = {255};
  const Bitmap shortData = {data,16,10,1};
  const Bitmap zeroWidth = {data,0,10,sizeof(data)};
  const Bitmap zeroHeight = {data,8,0,sizeof(data)};
  const Bitmap nullData = {nullptr,8,8,8};
  assert(!shortData.valid() && !zeroWidth.valid() && !zeroHeight.valid() && !nullData.valid());
  assert(!statusBarSetBitmap("battery",&shortData));
  const Bitmap replacement = {data,8,1,sizeof(data)};
  assert(statusBarSetBitmap("battery",&replacement));
  clear(); assert(statusBarDrawIcons(128,20,pixel)==98);
  assert(pixels[9][118] && pixels[9][125]);
  assert(statusBarResetIcon("battery"));
  statusBarResetIcons(); clear();
  memset(expected,0,sizeof(expected));
  expectedBitmap(expected,status_bitmap_battery,110);
  expectedBitmap(expected,status_bitmap_wifi,94);
  expectedBitmap(expected,status_bitmap_tf,78);
  assert(statusBarDrawIcons(128,20,pixel)==74);
  assert(memcmp(pixels,expected,sizeof(pixels))==0);
  assert(statusBarSetVisible("battery",false));
  assert(statusBarSetVisible("wifi",false));
  assert(statusBarSetVisible("tf",false));
  clear(); assert(statusBarDrawIcons(128,20,pixel)==128);
  bool empty[20][128] = {}; assert(memcmp(pixels,empty,sizeof(pixels))==0);
  statusBarResetIcons();
  // Oversized assets, short buffer and narrow screen must never write outside bar.
  const Bitmap tall = {data,8,20,sizeof(data)};
  const Bitmap wide = {data,128,1,sizeof(data)};
  StatusIconEntry entries[] = {
    {"tall",&tall,true,&tall,true}, {"wide",&wide,true,&wide,true},
    {"bad",&shortData,true,&shortData,true},
    {"small",&replacement,true,&replacement,true}};
  StatusIconLayout test(entries,4);
  clear(); assert(test.draw(128,20,pixel)==114);
  clear(); assert(test.draw(20,20,pixel)==20);
  assert(memcmp(pixels,empty,sizeof(pixels))==0);
  assert(test.draw(0,20,pixel)==0 && test.draw(128,0,pixel)==0);
  assert(test.draw(128,20,nullptr)==0);
  StatusIconLayout noEntries(nullptr,0);
  assert(noEntries.draw(128,20,pixel)==128);
  puts("PASS: original pixels, visibility, replacement, reset, malformed assets, overflow and clipping");
}
