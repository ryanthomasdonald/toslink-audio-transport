#include "DisplayUI.h"
#include "AudioEngine.h"
#include "LibraryCommon.h"  // 🚀 FIXED: Links back to master architecture states
#include <Wire.h>
#include <SD.h>

#define TFT_DC 8
#define TFT_CS 10
#define TFT_RST 9
#define FT6336U_ADDR 0x38

ST7796_t3 tft = ST7796_t3(TFT_CS, TFT_DC, TFT_RST);

uint32_t lastUpdatedSecond = 999999;
uint16_t lastProgressPixelWidth = 0;

uint16_t touchX = 0;
uint16_t touchY = 0;

const int pBarX = 198;
const int pBarY = 175;
const int pBarMaxWidth = 254;
const int pBarHeight = 8;

UI_Button transport[] = {
  { 10, 245, 85, 55, "PREV", 0x3186, false },
  { 105, 245, 85, 55, "PLAY", 0x03E0, false },
  { 200, 245, 85, 55, "STOP", ST7735_RED, false },
  { 295, 245, 85, 55, "NEXT", 0x3186, false },
  { 390, 245, 80, 55, "BROWSE", 0x5AAA, false }
};

void initDisplaySystem() {
  Wire.begin();
  Wire.setClock(400000);
  tft.init(320, 480);
  tft.invertDisplay(true);
  tft.setRotation(1);
}

void resetProgressTrackers() {
  lastUpdatedSecond = 999999;
  lastProgressPixelWidth = 0;
  if (currentUIState == STATE_PLAYER) {
    tft.fillRect(pBarX, pBarY, pBarMaxWidth, pBarHeight, 0x2104);
  }
}

void updatePlayPauseButtonLabel(const char* newLabel, uint16_t newColor) {
  transport[1].label = newLabel;
  transport[1].color = newColor;
  if (currentUIState == STATE_PLAYER) {
    drawTransportButton(transport[1]);
  }
}

void drawAudioDashboard() {
  tft.fillScreen(0x10A2);
  tft.drawRoundRect(15, 15, 450, 200, 8, ST7735_WHITE);
  tft.drawRect(22, 25, 160, 160, 0x52AA);
  tft.fillRect(pBarX, pBarY, pBarMaxWidth, pBarHeight, 0x2104);

  for (int i = 0; i < 5; i++) drawTransportButton(transport[i]);
  drawAlbumArtwork();
}

void drawAlbumArtwork() {
  // Read purely from our isolated 51KB dynamic RAM frame cache
  if (activeArtworkLoaded) {
    tft.writeRect(22, 25, 160, 160, activeArtworkCache);
  } else {
    // Elegant fallback placeholder box if no image was cached yet
    tft.fillRect(22, 25, 160, 160, 0x2104);
    tft.drawRect(22, 25, 160, 160, 0x52AA);
  }
}

void updateTrackWindow(int trackNum, const char* trackTitle) {
  if (currentUIState != STATE_PLAYER) return;

  tft.fillRect(198, 25, 258, 140, 0x10A2);
  resetProgressTrackers();

  String artistStr = String(currentArtistFolder);
  artistStr.replace("/", "");

  String albumStr = String(currentAlbumFolder);
  albumStr.replace("/", "");

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  tft.setCursor(198, 25);
  tft.print(artistStr.c_str());
  tft.setCursor(198, 37);
  tft.print(albumStr.c_str());

  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(198, 60);
  tft.printf("TRACK %02d OF %02d", trackNum, totalTracks);

  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.setCursor(198, 90);

  String cleanName = String(trackTitle);
  if (cleanName.length() > 3) {
    cleanName = cleanName.substring(3);
  }
  if (cleanName.endsWith(".wav") || cleanName.endsWith(".WAV")) {
    cleanName = cleanName.substring(0, cleanName.length() - 4);
  }

  tft.print(cleanName.c_str());
}

void drawTransportButton(UI_Button btn) {
  if (btn.isPressed) {
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 10, ST7735_WHITE);
    tft.setTextColor(ST7735_BLACK);
  } else {
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 10, btn.color);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 10, ST7735_WHITE);
    tft.setTextColor(ST7735_WHITE);
  }
  tft.setTextSize(1);
  if (strcmp(btn.label, "BROWSE") == 0) tft.setTextSize(1);
  else tft.setTextSize(2);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(btn.label, btn.x, btn.y, &x1, &y1, &w, &h);
  tft.setCursor(btn.x + (btn.w - w) / 2, btn.y + (btn.h - h) / 2 + 4);
  tft.print(btn.label);
}

void handleLiveTimeAndProgressBar() {
  if (currentUIState != STATE_PLAYER) return;

  uint32_t currentMs = activeEngineIsA ? playWav1.positionMillis() : playWav2.positionMillis();
  uint32_t totalMs = activeEngineIsA ? playWav1.lengthMillis() : playWav2.lengthMillis();
  if (totalMs == 0) return;

  uint32_t totalSeconds = currentMs / 1000;
  if (totalSeconds != lastUpdatedSecond) {
    lastUpdatedSecond = totalSeconds;
    uint32_t currentMins = totalSeconds / 60;
    uint32_t currentSecs = totalSeconds % 60;

    uint32_t totalTrackSeconds = totalMs / 1000;
    uint32_t totalTrackMins = totalTrackSeconds / 60;
    uint32_t totalTrackSecs = totalTrackSeconds % 60;

    tft.setTextColor(ST7735_CYAN, 0x10A2);
    tft.setTextSize(1);
    tft.setCursor(198, 155);
    tft.printf("%02lu:%02lu / %02lu:%02lu", currentMins, currentSecs, totalTrackMins, totalTrackSecs);
  }

  uint16_t newPixelWidth = ((float)currentMs / (float)totalMs) * pBarMaxWidth;
  if (newPixelWidth != lastProgressPixelWidth) {
    if (newPixelWidth > lastProgressPixelWidth) {
      tft.fillRect(pBarX + lastProgressPixelWidth, pBarY, newPixelWidth - lastProgressPixelWidth, pBarHeight, ST7735_GREEN);
    } else {
      tft.fillRect(pBarX, pBarY, pBarMaxWidth, pBarHeight, 0x2104);
    }
    lastProgressPixelWidth = newPixelWidth;
  }
}

bool readTouchPanel(uint16_t& x, uint16_t& y) {
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(true) != 0) return false;

  int bytesReceived = Wire.requestFrom(FT6336U_ADDR, 5);
  if (bytesReceived != 5) return false;

  uint8_t td_status = Wire.read();
  uint8_t p1_xh = Wire.read();
  uint8_t p1_xl = Wire.read();
  uint8_t p1_yh = Wire.read();
  uint8_t p1_yl = Wire.read();

  if ((td_status & 0x0F) == 0) return false;

  uint16_t rawX = ((uint16_t)(p1_xh & 0x0F) << 8) | p1_xl;
  uint16_t rawY = ((uint16_t)(p1_yh & 0x0F) << 8) | p1_yl;

  x = rawY;
  y = 320 - rawX;
  return true;
}

void processTouchControls() {
  if (currentUIState == STATE_MENU) {
    processMenuTouch();
    return;
  }

  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);

  if (currentTouch && !lastTouchState) {
    for (int i = 0; i < 5; i++) {
      if (touchX >= transport[i].x && touchX <= (transport[i].x + transport[i].w) && touchY >= transport[i].y && touchY <= (transport[i].y + transport[i].h)) {

        transport[i].isPressed = true;
        drawTransportButton(transport[i]);

        if (i == 0) {  // PREV
          if (currentTrackIndex > 0) {
            playWav1.stop();
            playWav2.stop();
            currentTrackIndex--;
            if (isMediaPlaying || isMediaPaused) playFreshAlbumStart();
            else updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          }
        } else if (i == 1) {  // PLAY / PAUSE
          if (!isMediaPlaying && !isMediaPaused) {
            playFreshAlbumStart();
          } else {
            if (activeEngineIsA) playWav1.togglePlayPause();
            else playWav2.togglePlayPause();

            if (isMediaPlaying) {
              isMediaPlaying = false;
              isMediaPaused = true;
              updatePlayPauseButtonLabel("PLAY", 0x03E0);
            } else {
              isMediaPlaying = true;
              isMediaPaused = false;
              updatePlayPauseButtonLabel("PAUSE", 0xD4A0);
            }
          }
        } else if (i == 2) {  // STOP
          if (isMediaPlaying || isMediaPaused) {
            playWav1.stop();
            playWav2.stop();
            isMediaPlaying = false;
            isMediaPaused = false;
            updatePlayPauseButtonLabel("PLAY", 0x03E0);
            updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          }
        } else if (i == 3) {  // NEXT
          if (currentTrackIndex < (totalTracks - 1)) {
            playWav1.stop();
            playWav2.stop();
            currentTrackIndex++;
            if (isMediaPlaying || isMediaPaused) playFreshAlbumStart();
            else updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          }
        } else if (i == 4) {  // BROWSE
          menuScrollOffset = 0;
          currentUIState = STATE_MENU;
          currentMenuLevel = LEVEL_ARTISTS;
          drawMenuScreen();
        }
      }
    }
  }

  if (!currentTouch && lastTouchState) {
    for (int i = 0; i < 5; i++) {
      if (transport[i].isPressed) {
        transport[i].isPressed = false;
        drawTransportButton(transport[i]);
      }
    }
  }
  lastTouchState = currentTouch;
}
