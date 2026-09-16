#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <ST7796_t3.h>

// =============================================================================
// 🚀 THE MASTER DIETER RAMS DARK MODE PALETTE DEFINITIONS
// =============================================================================
#define COLOR_RAMS_BG 0x0841         // Deep Charcoal Base
#define COLOR_RAMS_CARD 0x10A2       // Mid-Charcoal Containers & Bottom Rail
#define COLOR_RAMS_DIVIDER 0x2104    // Low-contrast Structural Separator Lines
#define COLOR_RAMS_TEXT_MUTE 0x7BEF  // Muted grey for supporting metadata
#define COLOR_RAMS_ORANGE 0xD4A0     // Active Signal Accents & Progress Bars
#define COLOR_RAMS_WHITE 0xFFFF      // Stark White for high-priority elements

struct UI_Button {
  int x, y, w, h;
  const char* label;
  uint16_t color;
  bool isPressed;
};

extern ST7796_t3 tft;
extern UI_Button transport[];
extern uint16_t touchX;
extern uint16_t touchY;

// Subsystem Control Functions
void initDisplaySystem();
void drawAudioDashboard();
void updateTrackWindow(int trackNum, const char* trackTitle);
void drawTransportButton(UI_Button btn);
void updatePlayPauseButtonLabel(const char* newLabel, uint16_t newColor);
void resetProgressTrackers();
void handleLiveTimeAndProgressBar();
void drawAlbumArtwork();
bool readTouchPanel(uint16_t& x, uint16_t& y);
void processTouchControls();
void drawBootLoadingScreen();
void drawWrappedTextLine(const char* text, int startX, int startY, int maxW, int fontScale, uint16_t color, uint16_t bgColor, int lineSpacing, int maxLines, int& outNextY);
void refreshDisplayHardwareState();

#endif
