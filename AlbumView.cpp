#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

// 🚀 EXPLICIT LAYER EXPOSURE
extern int browseArtistIndex;
extern int browseAlbumIndex;

void drawAlbumView() {
  refreshDisplayHardwareState();
  tft.fillScreen(COLOR_RAMS_BG);
  int dummyNextY = 0;

  tft.fillRect(0, 0, 480, 45, COLOR_RAMS_CARD);
  tft.drawFastHLine(0, 45, 480, COLOR_RAMS_DIVIDER);
  drawWrappedTextLine("SELECT ALBUM", 15, 19, 320, 2, COLOR_RAMS_ORANGE, COLOR_RAMS_CARD, 0, 1, dummyNextY);

  tft.fillRect(360, 45, 120, 92, COLOR_RAMS_CARD);
  tft.drawRect(360, 45, 120, 92, COLOR_RAMS_DIVIDER);
  drawWrappedTextLine("BACK", 396, 81, 100, 2, COLOR_RAMS_WHITE, COLOR_RAMS_CARD, 0, 1, dummyNextY);

  int midX = 360 + (120 / 2);
  int midY1 = 137 + (92 / 2);
  tft.fillRect(360, 137, 120, 92, COLOR_RAMS_CARD);
  tft.drawRect(360, 137, 120, 92, COLOR_RAMS_DIVIDER);
  tft.fillTriangle(midX, midY1 - 10, midX - 12, midY1 + 6, midX + 12, midY1 + 6, COLOR_RAMS_WHITE);

  int midY2 = 229 + (91 / 2);
  tft.fillRect(360, 229, 120, 91, COLOR_RAMS_CARD);
  tft.drawRect(360, 229, 120, 91, COLOR_RAMS_DIVIDER);
  tft.fillTriangle(midX, midY2 + 10, midX - 12, midY2 - 6, midX + 12, midY2 - 6, COLOR_RAMS_WHITE);

  // 🚀 THE REDIRECTION FIX: Pull file maps relative to browse registers to shield playing data
  int albumCount = library[browseArtistIndex].albumCount;

  if (albumCount == 0) {
    drawWrappedTextLine("EMPTY", 30, 120, 200, 2, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyNextY);
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      int boxY = 45 + (i * 55);

      if (itemIndex >= albumCount) {
        tft.fillRect(0, boxY, 360, 55, COLOR_RAMS_BG);
        tft.drawRect(0, boxY, 360, 55, COLOR_RAMS_DIVIDER);
        continue;
      }

      tft.fillRect(0, boxY, 360, 55, COLOR_RAMS_CARD);
      tft.drawRect(0, boxY, 360, 55, COLOR_RAMS_DIVIDER);
      tft.fillRect(12, boxY + 24, 8, 8, COLOR_RAMS_ORANGE);

      String nameStr = "";
      if (library[browseArtistIndex].albums[itemIndex].name != NULL) {
        nameStr = String(library[browseArtistIndex].albums[itemIndex].name);
      }
      nameStr.toUpperCase();
      drawWrappedTextLine(nameStr.c_str(), 34, boxY + 22, 310, 2, COLOR_RAMS_WHITE, COLOR_RAMS_CARD, 0, 1, dummyNextY);
    }
  }
}

void processAlbumViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  int albumCount = library[browseArtistIndex].albumCount;

  if (currentTouch && !lastTouchState) {
    if (touchX >= 360 && touchX <= 480) {
      if (touchY >= 45 && touchY <= 136) {  // BACK
        menuScrollOffset = 0;
        currentMenuLevel = LEVEL_ARTISTS;
        drawArtistView();
      } else if (touchY >= 137 && touchY <= 228) {  // PG UP
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawAlbumView();
        }
      } else if (touchY >= 229 && touchY <= 320) {  // PG DN
        if (menuScrollOffset + 5 < albumCount) {
          menuScrollOffset += 5;
          drawAlbumView();
        }
      }
    }
    if (touchX >= 0 && touchX <= 360) {
      for (int i = 0; i < 5; i++) {
        int itemIndex = menuScrollOffset + i;
        if (itemIndex >= albumCount) break;
        int boxY = 45 + (i * 55);
        if (touchY >= boxY && touchY <= (boxY + 55)) {
          // 🚀 THE SHIELD FIX: Track selections into browse register cache
          browseAlbumIndex = itemIndex;
          menuScrollOffset = 0;
          currentMenuLevel = LEVEL_TRACKS;
          drawTrackView();
          break;
        }
      }
    }
  }
  lastTouchState = currentTouch;
}
