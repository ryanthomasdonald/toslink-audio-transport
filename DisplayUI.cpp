#include "DisplayUI.h"
#include "AudioEngine.h"
#include "LibraryCommon.h"
#include "BespokeFont.h"  // 🚀 HOOK UP OUR CUSTOM ROW-MAJOR BITMAP DICTIONARY HERE
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

// Enforce a strict hardware bus timeout to prevent long line freezes
#if defined(TEENSYDUINO)
  Wire.setTimeout(3000);
#endif

  // Send a safe, native empty transmission to the touch chip to clear its buffer
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Initialize standard display controls cleanly
  tft.init(320, 480);
  tft.invertDisplay(true);
  tft.setRotation(1);
}

void drawBootLoadingScreen() {
  tft.fillScreen(COLOR_RAMS_BG);  // 0x0841 Deep Charcoal Base

  // Revert temporarily to system font for simple loading rendering
  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_RAMS_WHITE);

  const char* msg = "LOADING...";
  int16_t x1, y1;
  uint16_t tw, th;
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &tw, &th);

  tft.setCursor((480 - tw) / 2, (320 - th) / 2);
  tft.print(msg);
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

void drawBespokeChar(char c, int x, int y, int scale, uint16_t fgColor, uint16_t bgColor) {
  // Convert lowercase inputs to uppercase on the fly for uniformity
  if (c >= 'a' && c <= 'z') c -= 32;

  // Safe boundary check: Out-of-bounds characters are rendered as a clean space block
  if (c < 32 || c > 90) {
    tft.fillRect(x, y, 6 * scale, 5 * scale, bgColor);
    return;
  }

  // 🚀 INVARIANT OFFSET MATH: Index calculations align with the continuous 32-90 array structure
  int charIndex = c - 32;
  int bitmapOffset = charIndex * 5;

  // Sweep down the 5 rows
  for (int row = 0; row < 5; row++) {
    // Read the packed row byte out of Flash memory via pgm_read_byte
    uint8_t rowBits = pgm_read_byte(&Bespoke5x5Bitmaps[bitmapOffset + row]);

    // Sweep across the 5 columns
    for (int col = 0; col < 5; col++) {
      // Read bits from Left to Right (Bit 7 down to Bit 3)
      bool bitIsActive = (rowBits & (0x80 >> col));
      uint16_t activePixelColor = bitIsActive ? fgColor : bgColor;

      tft.fillRect(x + (col * scale), y + (row * scale), scale, scale, activePixelColor);
    }
  }

  // Render the 1-pixel wide tracking padding spacer trail
  tft.fillRect(x + (5 * scale), y, scale, 5 * scale, bgColor);
}

// =============================================================================
// 🚀 WORD-WRAPPING ENGINE RE-CONFIGURED FOR ROW-MAJOR ATOMIC TYPOGRAPHY
// =============================================================================
void drawWrappedTextLine(const char* text, int startX, int startY, int maxW, int fontScale, uint16_t color, uint16_t bgColor, int lineSpacing, int maxLines, int& outNextY) {
  String source = String(text);
  int currentX = startX;
  int currentY = startY;
  int lineCount = 1;

  // Explicitly define dimensions: 5px glyph + 1px spacing multiplied by scale
  int charW = 6 * fontScale;
  int charH = 5 * fontScale;
  int spaceWidth = 4 * fontScale;

  int wordStart = 0;
  bool firstWordOnLine = true;

  while (wordStart < (int)source.length()) {
    // Line constraint ceiling checks
    if (maxLines > 0 && lineCount >= maxLines && currentX + (spaceWidth * 3) > startX + maxW) {
      for (int e = 0; e < 3; e++) {
        drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor);
        currentX += charW;
      }
      break;
    }

    int wordEnd = source.indexOf(' ', wordStart);
    if (wordEnd == -1) wordEnd = source.length();

    String word = source.substring(wordStart, wordEnd);
    int wordW = word.length() * charW;
    if (word.length() > 0) wordW -= fontScale;  // Drop the final trailing spacer bit

    int requiredWidth = wordW + (firstWordOnLine ? 0 : spaceWidth);

    if (currentX + requiredWidth > startX + maxW) {
      if (!firstWordOnLine || wordW <= maxW) {
        if (maxLines > 0 && lineCount >= maxLines) {
          drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor);
          break;
        }
        currentX = startX;
        currentY += charH + lineSpacing;
        lineCount++;
        firstWordOnLine = true;
        requiredWidth = wordW;
      } else {
        // Character wrapping fallback block for extra long filenames
        for (int c = 0; c < (int)word.length(); c++) {
          if (maxLines > 0 && lineCount >= maxLines && currentX + charW > startX + maxW) {
            drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor);
            wordStart = source.length();
            break;
          }
          if (currentX + charW > startX + maxW) {
            if (maxLines > 0 && lineCount >= maxLines) {
              drawBespokeChar('.', currentX, currentY, fontScale, color, bgColor);
              wordStart = source.length();
              break;
            }
            currentX = startX;
            currentY += charH + lineSpacing;
            lineCount++;
          }
          drawBespokeChar(word[c], currentX, currentY, fontScale, color, bgColor);
          currentX += charW;
        }
        wordStart = wordEnd + 1;
        firstWordOnLine = false;
        continue;
      }
    }

    if (!firstWordOnLine) {
      tft.fillRect(currentX, currentY, spaceWidth, charH, bgColor);
      currentX += spaceWidth;
    }

    // Output word token array structures character-by-character
    for (int i = 0; i < (int)word.length(); i++) {
      drawBespokeChar(word[i], currentX, currentY, fontScale, color, bgColor);
      currentX += charW;
    }
    firstWordOnLine = false;
    wordStart = wordEnd + 1;
  }

  outNextY = currentY + charH;
}

void testBespokeString(const char* text, int startX, int startY, int scale, uint16_t fg, uint16_t bg) {
  int currentX = startX;
  int charStride = 6 * scale;

  int len = strlen(text);
  for (int i = 0; i < len; i++) {
    drawBespokeChar(text[i], currentX, startY, scale, fg, bg);
    currentX += charStride;
  }
}

// =============================================================================
// 🚀 METADATA TRACK WINDOW - RECONFIGURED FOR SCALE 3 HEADER & SCALE 1 METRICS
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

  // Clear out the master right column text viewport area
  tft.fillRect(235, 15, 230, 195, COLOR_RAMS_BG);

  int trackingY = 18;

  // 1. ARTIST NAME LINE (Scale 2 = Crisp 10px tall grid letters)
  int artistNextY = 0;
  drawWrappedTextLine(artistStr.c_str(), 235, trackingY, 230, 2, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 4, 2, artistNextY);

  // 2. ALBUM TITLE LINE (Scale 2 = Crisp 10px tall grid letters)
  trackingY = artistNextY + 8;
  int albumNextY = 0;
  drawWrappedTextLine(albumStr.c_str(), 235, trackingY, 230, 2, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 4, 2, albumNextY);

  // 3. LOW-CONTRAST SEPARATOR HORIZONTAL GRID LINE
  tft.drawFastHLine(235, 62, 230, COLOR_RAMS_DIVIDER);

  // Process track filename tags cleanly
  String cleanName = String(trackTitle);
  if (cleanName.length() > 3) cleanName = cleanName.substring(3);
  if (cleanName.endsWith(".wav") || cleanName.endsWith(".WAV")) {
    cleanName = cleanName.substring(0, cleanName.length() - 4);
  }
  cleanName.toUpperCase();

  // 4. THE ACTIVE HEADLINE SONG TITLE (🚀 SHRUNK TO SCALE 3 = 15px tall typography)
  trackingY = 76;  // Moves up beautifully now that the space is clear
  int titleNextY = 0;

  // Targeted clear of the specific title viewport slot envelope box
  tft.fillRect(235, 76, 230, 60, COLOR_RAMS_BG);
  drawWrappedTextLine(cleanName.c_str(), 235, trackingY, 230, 3, COLOR_RAMS_WHITE, COLOR_RAMS_BG, 6, 3, titleNextY);

  // 5. 🚀 HARMONIZED TRACK COUNTER (Moved down to the time tracking metric axis line)
  // Left-aligned at X = 235, drawn at Scale 1 (5pt)
  char countBuf[24];
  sprintf(countBuf, "TRACK %02d OF %02d", trackNum, totalTracks);
  int dummyCountY = 0;
  drawWrappedTextLine(countBuf, 235, 216, 120, 1, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyCountY);
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
  } else {  // System fallback for string buttons like "MENU"
            // =========================================================================
    // 🚀 HARMONIZED STRING BUTTON DRAW (MENU BUTTON)
    // Re-configured to route cleanly through our custom Scale 2 (10pt) 5x5 engine!
    // =========================================================================

    // 5px character + 1px trailing pad = 6px stride * 4 chars ("MENU") = 24px wide footprint
    // Scale 2 multiplies this to exactly 48px wide total
    int stringWidth = 48;
    int stringHeight = 10;  // 5px * Scale 2

    // Mathematically center the 48x10 text box inside the 96x70 button canvas bounds
    int textX = btn.x + ((btn.w - stringWidth) / 2);
    int textY = btn.y + ((btn.h - stringHeight) / 2);

    int dummyNextY = 0;

    // Draw the text using the exact button foreground (fg) and background (bg) states
    drawWrappedTextLine(btn.label, textX, textY, stringWidth + 2, 2, fg, bg, 0, 1, dummyNextY);
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

    // 🚀 PRO MATH: Calculate precise tracking layout width for Scale 1 bespoke font
    // 5px character content + 1px spacing trailing pad = 6px stride per character.
    int len = strlen(timeBuf);
    int timeStringWidth = (len * 6) - 1;  // Snug rightmost bit-edge clearance

    // 🚀 THE HARMONIZATION FIX: Instead of tft.print, route it through our 5x5 engine!
    // Draws right-justified at X = 465 minus our exact calculated font layout width
    int timeX = 465 - timeStringWidth;
    int dummyTimeY = 0;

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
            tft.setFont(NULL);
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