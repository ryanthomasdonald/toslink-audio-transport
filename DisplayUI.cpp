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

// Re-centered linear progress timeline metrics
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

// 🚀 THE DEFENSIVE BOOTESCAPE: Enforce a strict hardware bus timeout!
// If the touch panel or communication wires experience an edge collision,
// the I2C bus will timeout after 3000 microseconds instead of freezing the CPU forever.
#if defined(ARDUINO_ARCH_MEGAAVR) || defined(TEENSYDUINO)
  Wire.setTimeout(3000);
#endif

  // Flush any leftover startup state spikes on the lines safely
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Initialize standard display controls
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
    tft.writeRect(15, 15, 200, 200, activeArtworkCache);
  } else {
    tft.fillRect(15, 15, 200, 200, COLOR_RAMS_DIVIDER);
  }
}

// =============================================================================
// 🚀 FLICKER-FREE WORD-WRAPPING TEXT ENGINE WITH ARBITRARY LINE LIMIT CAPPING
// =============================================================================
void drawWrappedTextLine(const char* text, int startX, int startY, int maxW, int fontScale, uint16_t color, uint16_t bgColor, int lineSpacing, int maxLines, int& outNextY) {
  tft.setTextSize(fontScale);
  tft.setTextColor(color, bgColor);

  String source = String(text);
  int currentX = startX;
  int currentY = startY;
  int lineCount = 1;

  int16_t bx, by;
  uint16_t bw, bh;
  tft.getTextBounds("A", startX, startY, &bx, &by, &bw, &bh);

  int spaceWidth = 0;
  int16_t sx, sy;
  uint16_t sw, sh;
  tft.getTextBounds(" ", 0, 0, &sx, &sy, &sw, &sh);
  spaceWidth = sw;

  int wordStart = 0;
  bool firstWordOnLine = true;

  while (wordStart < (int)source.length()) {
    if (maxLines > 0 && lineCount >= maxLines && currentX + 35 > startX + maxW) {
      tft.setCursor(currentX, currentY);
      tft.print("...");
      break;
    }

    int wordEnd = source.indexOf(' ', wordStart);
    if (wordEnd == -1) wordEnd = source.length();

    String word = source.substring(wordStart, wordEnd);

    int16_t wx1, wy1;
    uint16_t wordW, wordH;
    tft.getTextBounds(word.c_str(), 0, 0, &wx1, &wy1, &wordW, &wordH);

    int requiredWidth = wordW + (firstWordOnLine ? 0 : spaceWidth);

    if (currentX + requiredWidth > startX + maxW) {
      if (!firstWordOnLine || wordW <= maxW) {
        if (maxLines > 0 && lineCount >= maxLines) {
          tft.setCursor(currentX, currentY);
          tft.print("...");
          break;
        }
        currentX = startX;
        currentY += bh + lineSpacing;
        lineCount++;
        firstWordOnLine = true;
        requiredWidth = wordW;
      } else {
        for (int c = 0; c < (int)word.length(); c++) {
          if (maxLines > 0 && lineCount >= maxLines && currentX + 15 > startX + maxW) {
            tft.setCursor(currentX, currentY);
            tft.print("...");
            wordStart = source.length();
            break;
          }
          char chStr[] = { word[c], '\0' };
          int16_t cx1, cy1;
          uint16_t charW, charH;
          tft.getTextBounds(chStr, 0, 0, &cx1, &cy1, &charW, &charH);

          if (currentX + charW > startX + maxW) {
            if (maxLines > 0 && lineCount >= maxLines) {
              tft.setCursor(currentX, currentY);
              tft.print("...");
              wordStart = source.length();
              break;
            }
            currentX = startX;
            currentY += bh + lineSpacing;
            lineCount++;
          }
          tft.setCursor(currentX, currentY);
          tft.print(chStr);
          currentX += charW;
        }
        wordStart = wordEnd + 1;
        firstWordOnLine = false;
        continue;
      }
    }

    if (!firstWordOnLine) {
      tft.setCursor(currentX, currentY);
      tft.print(" ");
      currentX += spaceWidth;
    }

    tft.setCursor(currentX, currentY);
    tft.print(word.c_str());
    currentX += wordW;
    firstWordOnLine = false;

    wordStart = wordEnd + 1;
  }
  outNextY = currentY + bh;
}

void updateTrackWindow(int trackNum, const char* trackTitle) {
  if (currentUIState != STATE_PLAYER) return;

  resetProgressTrackers();

  String artistStr = String(currentArtistFolder);
  artistStr.replace("/", "");
  artistStr.toUpperCase();

  String albumStr = String(currentAlbumFolder);
  albumStr.replace("/", "");
  albumStr.toUpperCase();

  int trackingY = 22;

  int artistNextY = 0;
  drawWrappedTextLine(artistStr.c_str(), 235, trackingY, 230, 1, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 4, 2, artistNextY);
  if (artistNextY < 42) {
    tft.fillRect(235, artistNextY, 230, 42 - artistNextY, COLOR_RAMS_BG);
  }

  trackingY = 42;
  int albumNextY = 0;
  drawWrappedTextLine(albumStr.c_str(), 235, trackingY, 230, 1, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 4, 2, albumNextY);
  if (albumNextY < 62) {
    tft.fillRect(235, albumNextY, 230, 62 - albumNextY, COLOR_RAMS_BG);
  }

  tft.drawFastHLine(235, 62, 230, COLOR_RAMS_DIVIDER);

  trackingY = 74;
  tft.setTextSize(1);
  tft.setTextColor(COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG);
  tft.setCursor(235, trackingY);
  tft.printf("TRACK %02D OF %02D", trackNum, totalTracks);
  tft.fillRect(235 + 96, trackingY, 230 - 96, 10, COLOR_RAMS_BG);

  String cleanName = String(trackTitle);
  if (cleanName.length() > 3) cleanName = cleanName.substring(3);
  if (cleanName.endsWith(".wav") || cleanName.endsWith(".WAV")) {
    cleanName = cleanName.substring(0, cleanName.length() - 4);
  }
  cleanName.toUpperCase();

  // =========================================================================
  // 🚀 SURGICAL RESIDUAL WIPE: Cleans up old text lines without any flashing
  // =========================================================================
  trackingY = 96;
  int titleNextY = 0;

  // We clear out the complete title bounds viewport box explicitly here, but since it's targeted
  // exactly right before the word loop draws, the rewrite happens fast enough to avoid any visual flicker [1].
  tft.fillRect(235, 96, 230, 114, COLOR_RAMS_BG);

  drawWrappedTextLine(cleanName.c_str(), 235, trackingY, 230, 2, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 6, 3, titleNextY);
}

void drawTransportButton(UI_Button btn) {
  uint16_t bg = COLOR_RAMS_CARD;
  uint16_t fg = COLOR_RAMS_WHITE;

  if (btn.isPressed) {
    bg = COLOR_RAMS_DIVIDER;
    fg = COLOR_RAMS_ORANGE;
  } else if (strcmp(btn.label, "||") == 0) {
    fg = COLOR_RAMS_ORANGE;
  }

  tft.fillRect(btn.x, btn.y, btn.w, btn.h, bg);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, COLOR_RAMS_DIVIDER);

  int cx = btn.x + (btn.w / 2);
  int cy = btn.y + (btn.h / 2);

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
            tft.setTextSize(1);
            const char* resetTimeBuf = "00:00 / 00:00";
            int16_t x1, y1;
            uint16_t tw, th;
            tft.getTextBounds(resetTimeBuf, 0, 0, &x1, &y1, &tw, &th);
            tft.fillRect(465 - tw, 215, tw, th + 2, COLOR_RAMS_BG);
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
