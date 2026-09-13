#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

void drawArtistView() {
  tft.fillScreen(0x0010);
  tft.fillRect(0, 0, 480, 45, 0x2104);
  tft.drawFastHLine(0, 45, 480, ST7735_WHITE);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(2);
  tft.setCursor(15, 13);
  tft.print("SELECT ARTIST");

  drawMenuSideButton(370, 60, 95, 65, "BACK", ST7735_RED);
  drawMenuSideButton(370, 140, 95, 65, "PG UP", 0x3186);
  drawMenuSideButton(370, 220, 95, 65, "PG DN", 0x3186);

  if (libraryArtistCount == 0) {
    tft.setTextColor(0x7BEF);
    tft.setCursor(30, 100);
    tft.print("EMPTY");
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      if (itemIndex >= libraryArtistCount) break;
      int itemBoxY = 60 + (i * 50);
      tft.fillRect(15, itemBoxY - 4, 340, 42, 0x10A2);
      tft.drawRoundRect(15, itemBoxY - 4, 340, 42, 4, 0x3186);
      tft.fillRect(25, itemBoxY + 13, 8, 8, 0x5AAA);
      tft.setTextColor(ST7735_WHITE);
      tft.setCursor(45, itemBoxY + 9);
      tft.print(library[itemIndex].name);
    }
  }
}

void processArtistViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  if (currentTouch && !lastTouchState) {
    if (touchX >= 370 && touchX <= 465) {
      if (touchY >= 60 && touchY <= 125) {  // BACK
        AudioNoInterrupts();
        currentUIState = STATE_PLAYER;
        drawAudioDashboard();
        extern uint32_t lastUpdatedSecond;
        extern uint16_t lastProgressPixelWidth;
        lastUpdatedSecond = 999999;
        lastProgressPixelWidth = 0;
        updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
        if (isMediaPlaying) updatePlayPauseButtonLabel("PAUSE", 0xD4A0);
        else updatePlayPauseButtonLabel("PLAY", 0x03E0);
        AudioInterrupts();
      } else if (touchY >= 140 && touchY <= 205) {
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawArtistView();
        }
      } else if (touchY >= 220 && touchY <= 285) {
        if (menuScrollOffset + 5 < libraryArtistCount) {
          menuScrollOffset += 5;
          drawArtistView();
        }
      }
    }
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      if (itemIndex >= libraryArtistCount) break;
      int itemBoxY = 60 + (i * 50);
      if (touchX >= 15 && touchX <= 355 && touchY >= (itemBoxY - 4) && touchY <= (itemBoxY + 38)) {
        selectedArtistIndex = itemIndex;
        menuScrollOffset = 0;
        currentMenuLevel = LEVEL_ALBUMS;  // 🚀 INSTANT RAM SWITCH
        drawAlbumView();
        break;
      }
    }
  }
  lastTouchState = currentTouch;
}
