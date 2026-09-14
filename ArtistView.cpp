#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

void drawArtistView() {
  tft.fillScreen(COLOR_RAMS_BG);

  // 1. Structural Header Block
  tft.fillRect(0, 0, 480, 45, COLOR_RAMS_CARD);
  tft.drawFastHLine(0, 45, 480, COLOR_RAMS_DIVIDER);
  tft.setTextColor(COLOR_RAMS_ORANGE);
  tft.setTextSize(2);
  tft.setCursor(15, 15);
  tft.print("SELECT ARTIST");

  // 2. PERFECT FULL-SCREEN GRID SIDE NAVIGATION STACK (Flush & Touching)
  // BACK Button (Top-Right Block: Exactly 92px tall)
  tft.fillRect(340, 45, 140, 92, COLOR_RAMS_CARD);
  tft.drawRect(340, 45, 140, 92, COLOR_RAMS_DIVIDER);
  tft.setTextColor(COLOR_RAMS_WHITE);
  tft.setTextSize(2);
  tft.setCursor(385, 81);
  tft.print("BACK");

  // PG UP Button (Middle-Right Block: Exactly 92px tall)
  tft.fillRect(340, 137, 140, 92, COLOR_RAMS_CARD);
  tft.drawRect(340, 137, 140, 92, COLOR_RAMS_DIVIDER);
  tft.setCursor(380, 173);
  tft.print("PG UP");

  // PG DN Button (Bottom-Right Block: Exactly 91px tall to hit bottom edge at 320)
  tft.fillRect(340, 229, 140, 91, COLOR_RAMS_CARD);
  tft.drawRect(340, 229, 140, 91, COLOR_RAMS_DIVIDER);
  tft.setCursor(380, 265);
  tft.print("PG DN");

  // 3. PIXEL-PERFECT LIST LAYER (5 items @ 55px completely fills 45 to 320)
  if (libraryArtistCount == 0) {
    tft.setTextColor(COLOR_RAMS_TEXT_MUTE);
    tft.setCursor(30, 120);
    tft.print("EMPTY");
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      int boxY = 45 + (i * 55);  // 55px vertical stride blocks

      if (itemIndex >= libraryArtistCount) {
        // Keep the visual structure solid for empty slots
        tft.fillRect(0, boxY, 340, 55, COLOR_RAMS_BG);
        tft.drawRect(0, boxY, 340, 55, COLOR_RAMS_DIVIDER);
        continue;
      }

      tft.fillRect(0, boxY, 340, 55, COLOR_RAMS_CARD);
      tft.drawRect(0, boxY, 340, 55, COLOR_RAMS_DIVIDER);

      // Centered accent signal indicator dot
      tft.fillRect(10, boxY + 24, 8, 8, COLOR_RAMS_ORANGE);

      String nameStr = String(library[itemIndex].name);
      nameStr.toUpperCase();

      // Dynamic width pixel calculation boundary checks
      int16_t x1, y1;
      uint16_t tw, th;
      tft.setTextSize(2);
      tft.getTextBounds(nameStr.c_str(), 32, boxY + 20, &x1, &y1, &tw, &th);
      while (tw > 290 && nameStr.length() > 4) {
        nameStr = nameStr.substring(0, nameStr.length() - 1);
        String testStr = nameStr + "...";
        tft.getTextBounds(testStr.c_str(), 32, boxY + 20, &x1, &y1, &tw, &th);
      }
      if (tw <= 290 && nameStr.length() < strlen(library[itemIndex].name)) {
        nameStr += "...";
      }

      tft.setTextColor(COLOR_RAMS_WHITE);
      tft.setCursor(32, boxY + 20);
      tft.print(nameStr.c_str());
    }
  }
}

void processArtistViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  if (currentTouch && !lastTouchState) {
    // 🚀 CRITICAL UPDATE: Navigation Column Grid-Hit Geometry
    if (touchX >= 340 && touchX <= 480) {
      if (touchY >= 45 && touchY <= 136) {  // BACK (45 to 136)
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
      } else if (touchY >= 137 && touchY <= 228) {  // PG UP (137 to 228)
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawArtistView();
        }
      } else if (touchY >= 229 && touchY <= 320) {  // PG DN (229 to 320)
        if (menuScrollOffset + 5 < libraryArtistCount) {
          menuScrollOffset += 5;
          drawArtistView();
        }
      }
    }

    // 🚀 CRITICAL UPDATE: List Content Rows Grid-Hit Geometry
    if (touchX >= 0 && touchX <= 340) {
      for (int i = 0; i < 5; i++) {
        int itemIndex = menuScrollOffset + i;
        if (itemIndex >= libraryArtistCount) break;

        int boxY = 45 + (i * 55);  // Adjusted to match 55px layout grid
        if (touchY >= boxY && touchY <= (boxY + 55)) {
          selectedArtistIndex = itemIndex;
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
