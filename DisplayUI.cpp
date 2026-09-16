#include "DisplayUI.h"
#include "AudioEngine.h"
#include "LibraryCommon.h"
#include "BespokeFont.h"
#include <Wire.h>
#include <SD.h>

// 🚀 HARDWARE 3-PIN SCREEN CONTROL ASSIGNMENTS
#define TFT_DC 8
#define TFT_CS 10
#define TFT_RST 9
#define FT6336U_ADDR 0x38

ST7796_t3 tft = ST7796_t3(TFT_CS, TFT_DC, TFT_RST);
uint32_t lastUpdatedSecond = 999999;
uint16_t lastProgressPixelWidth = 0;
uint16_t touchX = 0;
uint16_t touchY = 0;

// Linear progress timeline metrics
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

#if defined(TEENSYDUINO)
  Wire.setTimeout(3000);
#endif

  // 🚀 THE STABILITY FIX: Pass the 30MHz speed override safely as the third argument!
  // This scales down the raw SPI bus clock transfer rate from its blistering default
  // down to a noise-resistant threshold that handles long jumper wires easily.
  tft.init(320, 480, 30000000);

  // Send a safe, native empty transmission to the touch chip
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(true);

  tft.invertDisplay(true);
  tft.setRotation(1);  // Force the initial orientation
}

// 🚀 NEW DEFENSIVE REFRESH: Call this at the start of drawMenuScreen()
// and updateTrackWindow() to "re-lock" the orientation and clear noise.
void refreshDisplayHardwareState() {
  // Re-assert the rotation and inversion registers.
  // If noise flipped the screen, this snaps it back instantly.
  tft.setRotation(1);
  tft.invertDisplay(true);
}

void drawBootLoadingScreen() {
  tft.fillScreen(COLOR_RAMS_BG);  // 0x0841 Deep Charcoal Base

  int startX = 180;
  int startY = 150;
  int dummyNextY = 0;

  // Render "LOADING..." using our polished 10pt (Scale 2) bespoke font
  drawWrappedTextLine("LOADING...", startX, startY, 200, 2, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 0, 1, dummyNextY);
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
// 🚀 NATIVE ROW-MAJOR ATOMIC 5X5 VARIABLE-WIDTH CHARACTER BLITTER
// =============================================================================
int getBespokeCharWidth(char c) {
  if (c == ' ') return 3;
  if (c == '.' || c == '!' || c == '\'' || c == ':' || c == ';') return 1;
  if (c == '(' || c == ')') return 2;
  if (c == '<' || c == '>') return 3;
  return 5;
}

void drawBespokeChar(char c, int x, int y, int scale, uint16_t fgColor, uint16_t bgColor, int& outRenderedWidth) {
  if (c >= 'a' && c <= 'z') c -= 32;

  int contentWidth = getBespokeCharWidth(c);
  int totalWidthWithPad = (contentWidth + 1) * scale;

  if (c < 32 || c > 90) {
    tft.fillRect(x, y, totalWidthWithPad, 5 * scale, bgColor);
    outRenderedWidth = totalWidthWithPad;
    return;
  }

  int charIndex = c - 32;
  int bitmapOffset = charIndex * 5;

  for (int row = 0; row < 5; row++) {
    uint8_t rowBits = pgm_read_byte(&Bespoke5x5Bitmaps[bitmapOffset + row]);

    for (int col = 0; col < contentWidth; col++) {
      bool bitIsActive = (rowBits & (0x80 >> col));
      uint16_t activePixelColor = bitIsActive ? fgColor : bgColor;

      tft.fillRect(x + (col * scale), y + (row * scale), scale, scale, activePixelColor);
    }
  }

  tft.fillRect(x + (contentWidth * scale), y, scale, 5 * scale, bgColor);
  outRenderedWidth = totalWidthWithPad;
}

// =============================================================================
// 🚀 WORD-WRAPPING ENGINE WITH VARIABLE-WIDTH TRACKING CORE
// =============================================================================
void drawWrappedTextLine(const char* text, int startX, int startY, int maxW, int fontScale, uint16_t color, uint16_t bgColor, int lineSpacing, int maxLines, int& outNextY) {
  String source = String(text);
  int currentX = startX;
  int currentY = startY;
  int lineCount = 1;

  int charH = 5 * fontScale;
  int spaceWidth = getBespokeCharWidth(' ') * fontScale;
  int dotWidth = (getBespokeCharWidth('.') + 1) * fontScale;

  int wordStart = 0;
  bool firstWordOnLine = true;

  while (wordStart < (int)source.length()) {
    if (maxLines > 0 && lineCount >= maxLines && currentX + (dotWidth * 3) > startX + maxW) {
      for (int e = 0; e < 3; e++) {
        int pad = 0;
        drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor, pad);
        currentX += pad;
      }
      break;
    }

    int wordEnd = source.indexOf(' ', wordStart);
    if (wordEnd == -1) wordEnd = source.length();

    String word = source.substring(wordStart, wordEnd);

    int wordW = 0;
    for (int i = 0; i < (int)word.length(); i++) {
      wordW += (getBespokeCharWidth(word[i]) + 1) * fontScale;
    }
    if (word.length() > 0) wordW -= fontScale;

    int requiredWidth = wordW + (firstWordOnLine ? 0 : spaceWidth + fontScale);

    if (currentX + requiredWidth > startX + maxW) {
      if (!firstWordOnLine || wordW <= maxW) {
        if (maxLines > 0 && lineCount >= maxLines) {
          for (int e = 0; e < 3; e++) {
            int pad = 0;
            drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor, pad);
            currentX += pad;
          }
          break;
        }
        currentX = startX;
        currentY += charH + lineSpacing;
        lineCount++;
        firstWordOnLine = true;
        requiredWidth = wordW;
      } else {
        for (int c = 0; c < (int)word.length(); c++) {
          int charPadW = 0;
          int nextCharW = (getBespokeCharWidth(word[c]) + 1) * fontScale;

          if (maxLines > 0 && lineCount >= maxLines && currentX + nextCharW > startX + maxW) {
            drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor, charPadW);
            wordStart = source.length();
            break;
          }
          if (currentX + nextCharW > startX + maxW) {
            if (maxLines > 0 && lineCount >= maxLines) {
              drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor, charPadW);
              wordStart = source.length();
              break;
            }
            currentX = startX;
            currentY += charH + lineSpacing;
            lineCount++;
          }
          drawBespokeChar(word[c], currentX, currentY, fontScale, color, bgColor, charPadW);
          currentX += charPadW;
        }
        wordStart = wordEnd + 1;
        firstWordOnLine = false;
        continue;
      }
    }

    if (!firstWordOnLine) {
      int spacePadW = 0;
      drawBespokeChar(' ', currentX, currentY, fontScale, color, bgColor, spacePadW);
      currentX += spacePadW;
    }

    for (int i = 0; i < (int)word.length(); i++) {
      int charPadW = 0;
      drawBespokeChar(word[i], currentX, currentY, fontScale, color, bgColor, charPadW);
      currentX += charPadW;
    }
    firstWordOnLine = false;
    wordStart = wordEnd + 1;
  }

  outNextY = currentY + charH;
}

void testBespokeString(const char* text, int startX, int startY, int scale, uint16_t fg, uint16_t bg) {
  int currentX = startX;
  for (int i = 0; i < (int)strlen(text); i++) {
    int charPadW = 0;
    drawBespokeChar(text[i], currentX, startY, scale, fg, bg, charPadW);
    currentX += charPadW;
  }
}

// =============================================================================
// 🚀 HERO TRACK METADATA WINDOW MANAGER
// =============================================================================
void updateTrackWindow(int trackNum, const char* trackTitle) {
  if (currentUIState != STATE_PLAYER) return;

  resetProgressTrackers();

  String artistStr = String(currentArtistFolder);
  artistStr.replace("/", "");
  artistStr.toUpperCase();

  String albumStr = String(currentAlbumFolder);
  albumStr.replace("/", "");
  albumStr.toUpperCase();

  // Clear out the master right column text viewport area completely
  tft.fillRect(235, 15, 230, 195, COLOR_RAMS_BG);

  int trackingY = 18;

  // 1. ARTIST NAME LINE (Scale 2 = 10px tall grid letters)
  int artistNextY = 0;
  drawWrappedTextLine(artistStr.c_str(), 235, trackingY, 230, 2, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 4, 2, artistNextY);

  // 2. ALBUM TITLE LINE (Scale 2 = 10px tall grid letters)
  trackingY = artistNextY + 8;
  int albumNextY = 0;
  drawWrappedTextLine(albumStr.c_str(), 235, trackingY, 230, 2, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 4, 2, albumNextY);  // 3. LOW-CONTRAST SEPARATOR HORIZONTAL GRID LINE
  tft.drawFastHLine(235, 62, 230, COLOR_RAMS_DIVIDER);                                                               // Process track filename tags cleanly
  String cleanName = String(trackTitle);
  if (cleanName.length() > 3) cleanName = cleanName.substring(3);
  if (cleanName.endsWith(".wav") || cleanName.endsWith(".WAV")) { cleanName = cleanName.substring(0, cleanName.length() - 4); }
  cleanName.toUpperCase();  // 4. THE ACTIVE HEADLINE SONG TITLE (LOCKED BACK TO FIXED SCALE 3 = 15pt)
  trackingY = 74;
  int titleNextY = 0;                                                                                                 // Enforce a strict maxLines limit of 3 with auto ellipsis truncation.
  drawWrappedTextLine(cleanName.c_str(), 235, trackingY, 230, 3, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 6, 3, titleNextY);  // 5. HARMONIZED TRACK COUNTER (Left-aligned baseline axis line at Y=216)
  char countBuf[24];
  sprintf(countBuf, "TRACK %02d OF %02d", trackNum, totalTracks);
  int dummyCountY = 0;
  drawWrappedTextLine(countBuf, 235, 216, 120, 1, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyCountY);
}  // =============================================================================// 🚀 TIMELINE METRICS CLOCK RE-RENDERER// =============================================================================
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
    sprintf(timeBuf, "%02lu:%02lu / %02lu:%02lu", currentMins, currentSecs, totalTrackMins, totalTrackSecs);  // Calculate tracking width safely
    int len = strlen(timeBuf);
    int timeStringWidth = 0;
    for (int i = 0; i < len; i++) { timeStringWidth += (getBespokeCharWidth(timeBuf[i]) + 1) * 1; }
    timeStringWidth -= 1;
    int timeX = 465 - timeStringWidth;
    int dummyTimeY = 0;  // Wipes just the time text box slot selectively right before blitting
    tft.fillRect(timeX - 2, 216, timeStringWidth + 4, 6, COLOR_RAMS_BG);
    drawWrappedTextLine(timeBuf, timeX, 216, timeStringWidth + 2, 1, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyTimeY);
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
  } else {  // HARMONIZED STRING BUTTON DRAW (MENU BUTTON)
    int stringWidth = 48;
    int stringHeight = 10;
    int textX = btn.x + ((btn.w - stringWidth) / 2);
    int textY = btn.y + ((btn.h - stringHeight) / 2);
    int dummyNextY = 0;
    drawWrappedTextLine(btn.label, textX, textY, stringWidth + 2, 2, fg, bg, 0, 1, dummyNextY);
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
            char resetTimeBuf[32] = "00:00 / 00:00";
            int len = strlen(resetTimeBuf);
            int timeStringWidth = 0;
            for (int k = 0; k < len; k++) { timeStringWidth += (getBespokeCharWidth(resetTimeBuf[k]) + 1) * 1; }
            timeStringWidth -= 1;
            int timeX = 465 - timeStringWidth;
            int dummyTimeY = 0;
            tft.fillRect(timeX - 2, 216, timeStringWidth + 4, 6, COLOR_RAMS_BG);
            drawWrappedTextLine(resetTimeBuf, timeX, 216, timeStringWidth + 2, 1, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyTimeY);
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