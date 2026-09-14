#include "DisplayUI.h"
#include "AudioEngine.h"
#include "LibraryCommon.h"
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

// 🚀 RE-CENTERED PROGRESS TIMELINE GEOMETRY (Full-width bar below the artwork frame card blocks)
const int pBarX = 15;
const int pBarY = 231;
const int pBarMaxWidth = 450;
const int pBarHeight = 4;

UI_Button transport[] = {
  { 0, 250, 96, 70, "<<", COLOR_RAMS_CARD, false },
  { 96, 250, 96, 70, ">", COLOR_RAMS_CARD, false },
  { 192, 250, 96, 70, "[]", COLOR_RAMS_CARD, false },
  { 288, 250, 96, 70, ">>", COLOR_RAMS_CARD, false },
  { 384, 250, 96, 70, "MENU", COLOR_RAMS_CARD, false }
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
    tft.fillRect(pBarX, pBarY, pBarMaxWidth, pBarHeight, COLOR_RAMS_DIVIDER);
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
  tft.fillScreen(COLOR_RAMS_BG);

  // Solid mechanical bottom rail spanning across full layout width
  tft.fillRect(0, 250, 480, 70, COLOR_RAMS_CARD);
  tft.drawFastHLine(0, 250, 480, COLOR_RAMS_DIVIDER);

  for (int i = 0; i < 5; i++) {
    drawTransportButton(transport[i]);
  }
  drawAlbumArtwork();
}

void drawAlbumArtwork() {
  if (activeArtworkLoaded) {
    // 🚀 Renders full-scale 200x200 unbordered block directly from RAM2
    tft.writeRect(15, 15, 200, 200, activeArtworkCache);
  } else {
    tft.fillRect(15, 15, 200, 200, COLOR_RAMS_DIVIDER);
  }
}

void updateTrackWindow(int trackNum, const char* trackTitle) {
  if (currentUIState != STATE_PLAYER) return;

  // 🚀 THE FIX: Purge the massive 230x195 fillRect block!
  // Instead, we only clear out the specific tracks area if needed,
  // or let the background text printing handle the metadata lines natively.
  resetProgressTrackers();

  String artistStr = String(currentArtistFolder);
  artistStr.replace("/", "");
  artistStr.toUpperCase();

  String albumStr = String(currentAlbumFolder);
  albumStr.replace("/", "");
  albumStr.toUpperCase();

  // 1. ARTIST NAME (Surgically clean up using background color drawing parameters)
  tft.setTextSize(1);
  tft.setTextColor(COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG);  // 🚀 Background color injected
  tft.setCursor(235, 22);
  tft.print(artistStr.c_str());

  // Clear any residual character tails on the row line padding safely
  int16_t x1, y1;
  uint16_t tw, th;
  tft.getTextBounds(artistStr.c_str(), 235, 22, &x1, &y1, &tw, &th);
  if (tw < 230) tft.fillRect(235 + tw, 22, 230 - tw, th, COLOR_RAMS_BG);

  // 2. ALBUM TITLE
  tft.setTextColor(COLOR_RAMS_WHITE, COLOR_RAMS_BG);  // 🚀 Background color injected
  tft.setTextSize(1);
  tft.setCursor(235, 42);
  tft.print(albumStr.c_str());

  tft.getTextBounds(albumStr.c_str(), 235, 42, &x1, &y1, &tw, &th);
  if (tw < 230) tft.fillRect(235 + tw, 42, 230 - tw, th, COLOR_RAMS_BG);

  // 3. SEPARATOR LINE
  tft.drawFastHLine(235, 62, 230, COLOR_RAMS_DIVIDER);

  // 4. METADATA METRICS COUNTER
  tft.setTextColor(COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG);  // 🚀 Background color injected
  tft.setTextSize(1);
  tft.setCursor(235, 80);
  tft.printf("TRACK %02D OF %02D", trackNum, totalTracks);

  tft.fillRect(235 + 96, 80, 230 - 96, 10, COLOR_RAMS_BG);  // Clear rest of counter track width line

  // Clean up title naming parameters
  String cleanName = String(trackTitle);
  if (cleanName.length() > 3) cleanName = cleanName.substring(3);
  if (cleanName.endsWith(".wav") || cleanName.endsWith(".WAV")) {
    cleanName = cleanName.substring(0, cleanName.length() - 4);
  }
  cleanName.toUpperCase();

  // 5. THE ACTIVE SONG TITLE
  // Because titles can change dramatically in size, we wipe just this bounding text slot box container
  tft.fillRect(235, 105, 230, 24, COLOR_RAMS_BG);  // 🚀 Surgical song line slot wipe only!
  tft.setTextColor(COLOR_RAMS_WHITE);
  tft.setTextSize(2);
  tft.setCursor(235, 105);
  tft.print(cleanName.c_str());
}

void drawTransportButton(UI_Button btn) {
  uint16_t bg = COLOR_RAMS_CARD;
  uint16_t fg = COLOR_RAMS_WHITE;

  // 🚀 TACTILE STATE OVERRIDE: Refined mechanical color feedback logic
  if (btn.isPressed) {
    bg = COLOR_RAMS_DIVIDER;  // Darken background tile layout slightly on press
    fg = COLOR_RAMS_ORANGE;   // The icon glows instantly in Signal Orange while your finger is down
  } else if (strcmp(btn.label, "||") == 0) {
    fg = COLOR_RAMS_ORANGE;  // Main active play accent state indicator
  }

  // Render the rigid flush tile container blocks
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, bg);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, COLOR_RAMS_DIVIDER);

  int cx = btn.x + (btn.w / 2);
  int cy = btn.y + (btn.h / 2);

  // Vector geometry parsing lines
  if (strcmp(btn.label, ">") == 0) {
    tft.fillTriangle(cx - 8, cy - 12, cx - 8, cy + 12, cx + 12, cy, fg);
  } else if (strcmp(btn.label, "||") == 0) {
    tft.fillRect(cx - 8, cy - 12, 5, 24, fg);
    tft.fillRect(cx + 3, cy - 12, 5, 24, fg);
  } else if (strcmp(btn.label, "[]") == 0) {
    tft.fillRect(cx - 10, cy - 10, 20, 20, fg);
  } else if (strcmp(btn.label, "<<") == 0) {
    tft.fillTriangle(cx, cy - 10, cx, cy + 10, cx - 10, cy, fg);
    tft.fillTriangle(cx + 10, cy - 10, cx + 10, cy + 10, cx, cy, fg);
  } else if (strcmp(btn.label, ">>") == 0) {
    tft.fillTriangle(cx, cy - 10, cx, cy + 10, cx + 10, cy, fg);
    tft.fillTriangle(cx - 10, cy - 10, cx - 10, cy + 10, cx, cy, fg);
  } else {
    tft.setTextColor(fg);
    tft.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(btn.label, btn.x, btn.y, &x1, &y1, &w, &h);
    tft.setCursor(cx - (w / 2), cy - (h / 2) + 2);
    tft.print(btn.label);
  }
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

    char timeBuf[32];
    sprintf(timeBuf, "%02lu:%02lu / %02lu:%02lu", currentMins, currentSecs, totalTrackMins, totalTrackSecs);

    tft.setTextSize(1);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds(timeBuf, 0, 0, &x1, &y1, &tw, &th);

    // 🚀 RIGHT JUSTIFIED MATH: Clear and paint clock exactly anchored at the right margin line (465)
    tft.fillRect(465 - tw, 215, tw, th + 2, COLOR_RAMS_BG);
    tft.setTextColor(COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG);
    tft.setCursor(465 - tw, 215);
    tft.print(timeBuf);
  }

  uint16_t newPixelWidth = ((float)currentMs / (float)totalMs) * pBarMaxWidth;
  if (newPixelWidth != lastProgressPixelWidth) {
    if (newPixelWidth > lastProgressPixelWidth) {
      tft.fillRect(pBarX + lastProgressPixelWidth, pBarY, newPixelWidth - lastProgressPixelWidth, pBarHeight, COLOR_RAMS_ORANGE);
    } else {
      tft.fillRect(pBarX, pBarY, pBarMaxWidth, pBarHeight, COLOR_RAMS_DIVIDER);
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
        if (i == 0) {
          if (currentTrackIndex > 0) {
            playWav1.stop();
            playWav2.stop();
            currentTrackIndex--;
            if (isMediaPlaying || isMediaPaused) playFreshAlbumStart();
            else updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          }
        } else if (i == 1) {
          if (!isMediaPlaying && !isMediaPaused) {
            playFreshAlbumStart();
          } else {
            if (activeEngineIsA) playWav1.togglePlayPause();
            else playWav2.togglePlayPause();
            if (isMediaPlaying) {
              isMediaPlaying = false;
              isMediaPaused = true;
              updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
            } else {
              isMediaPlaying = true;
              isMediaPaused = false;
              updatePlayPauseButtonLabel("||", COLOR_RAMS_CARD);
            }
          }
        } else if (i == 2) {
          if (isMediaPlaying || isMediaPaused) {
            playWav1.stop();
            playWav2.stop();
            isMediaPlaying = false;
            isMediaPaused = false;
            updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
            // 🚀 THE COUNTER RESET FIX: Explicitly zero out the live clock viewport
            tft.setTextSize(1);
            const char* resetTimeBuf = "00:00 / 00:00";
            int16_t x1, y1;
            uint16_t tw, th;
            tft.getTextBounds(resetTimeBuf, 0, 0, &x1, &y1, &tw, &th);

            // Clear the tiny rectangle slot right above the progress bar line
            tft.fillRect(465 - tw, 215, tw, th + 2, COLOR_RAMS_BG);

            // Render the fresh zero baseline
            tft.setTextColor(COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG);
            tft.setCursor(465 - tw, 215);
            tft.print(resetTimeBuf);
            updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          }
        } else if (i == 3) {
          if (currentTrackIndex < (totalTracks - 1)) {
            playWav1.stop();
            playWav2.stop();
            currentTrackIndex++;
            if (isMediaPlaying || isMediaPaused) playFreshAlbumStart();
            else updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          }
        } else if (i == 4) {
          menuScrollOffset = 0;
          currentUIState = STATE_MENU;
          currentMenuLevel = LEVEL_ARTISTS;
          drawMenuScreen();
        }
      }
    }
  }
  if (!lastTouchState && currentTouch) {  // Safe catcher for inverse edge anomalies
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