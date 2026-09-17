#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

// 🚀 EXPLICIT LAYER EXPOSURE
extern int browseArtistIndex;

void drawArtistView() {
  refreshDisplayHardwareState();
  tft.fillScreen(COLOR_RAMS_BG);

  int dummyNextY = 0;

  // 1. Structural Header Block
  tft.fillRect(0, 0, 480, 45, COLOR_RAMS_CARD);
  tft.drawFastHLine(0, 45, 480, COLOR_RAMS_DIVIDER);
  drawWrappedTextLine("SELECT ARTIST", 15, 19, 320, 2, COLOR_RAMS_ORANGE, COLOR_RAMS_CARD, 0, 1, dummyNextY);

  // =========================================================================
  // 2. INDUSTRIAL ACCENT NAVIGATION COLUMN (120px WIDE AXIS)
  // =========================================================================
  tft.fillRect(360, 45, 120, 92, COLOR_RAMS_CARD);
  tft.drawRect(360, 45, 120, 92, COLOR_RAMS_DIVIDER);
  drawWrappedTextLine("BACK", 396, 86, 100, 2, COLOR_RAMS_WHITE, COLOR_RAMS_CARD, 0, 1, dummyNextY);

  int midX = 360 + (120 / 2);  // 420
  int midY1 = 137 + (92 / 2);  // 183
  tft.fillRect(360, 137, 120, 92, COLOR_RAMS_CARD);
  tft.drawRect(360, 137, 120, 92, COLOR_RAMS_DIVIDER);
  tft.fillTriangle(midX, midY1 - 10, midX - 12, midY1 + 6, midX + 12, midY1 + 6, COLOR_RAMS_WHITE);

  int midY2 = 229 + (91 / 2);  // 274
  tft.fillRect(360, 229, 120, 91, COLOR_RAMS_CARD);
  tft.drawRect(360, 229, 120, 91, COLOR_RAMS_DIVIDER);
  tft.fillTriangle(midX, midY2 + 10, midX - 12, midY2 - 6, midX + 12, midY2 - 6, COLOR_RAMS_WHITE);

  // =========================================================================
  // 3. PIXEL-PERFECT LIST LAYER (EXPANDED TO 360px WIDE)
  // =========================================================================
  if (libraryArtistCount == 0) {
    drawWrappedTextLine("EMPTY", 30, 120, 200, 2, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyNextY);
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      int boxY = 45 + (i * 55);

      if (itemIndex >= libraryArtistCount) {
        tft.fillRect(0, boxY, 360, 55, COLOR_RAMS_BG);
        tft.drawRect(0, boxY, 360, 55, COLOR_RAMS_DIVIDER);
        continue;
      }

      tft.fillRect(0, boxY, 360, 55, COLOR_RAMS_CARD);
      tft.drawRect(0, boxY, 360, 55, COLOR_RAMS_DIVIDER);
      tft.fillRect(12, boxY + 24, 8, 8, COLOR_RAMS_ORANGE);

      String nameStr = String(library[itemIndex].name);
      nameStr.toUpperCase();

      drawWrappedTextLine(nameStr.c_str(), 34, boxY + 22, 310, 2, COLOR_RAMS_WHITE, COLOR_RAMS_CARD, 0, 1, dummyNextY);
    }
  }
}

void processArtistViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  if (currentTouch && !lastTouchState) {
    if (touchX >= 360 && touchX <= 480) {
      if (touchY >= 45 && touchY <= 136) {  // BACK
        AudioNoInterrupts();
        menuScrollOffset = 0;
        currentUIState = STATE_PLAYER;
        drawAudioDashboard();
        extern uint32_t lastUpdatedSecond;
        extern uint16_t lastProgressPixelWidth;
        lastUpdatedSecond = 999999;
        lastProgressPixelWidth = 0;
        updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
        if (isMediaPlaying) updatePlayPauseButtonLabel("||", COLOR_RAMS_CARD);
        else updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
        AudioInterrupts();
      } else if (touchY >= 137 && touchY <= 228) {  // PG UP
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawArtistView();
        }
      } else if (touchY >= 229 && touchY <= 320) {  // PG DN
        if (menuScrollOffset + 5 < libraryArtistCount) {
          menuScrollOffset += 5;
          drawArtistView();
        }
      }
    }

    if (touchX >= 0 && touchX <= 360) {
      for (int i = 0; i < 5; i++) {
        int itemIndex = menuScrollOffset + i;
        if (itemIndex >= libraryArtistCount) break;

        int boxY = 45 + (i * 55);
        if (touchY >= boxY && touchY <= (boxY + 55)) {
          // 🚀 THE CRITICAL FIX: Lock the target index into the independent browse cache!
          browseArtistIndex = itemIndex;
          menuScrollOffset = 0;
          currentMenuLevel = LEVEL_ALBUMS;
          drawAlbumView();
          break;
        }
      }
    }
  }
  lastTouchState = currentTouch;
}
