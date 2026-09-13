#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
// 🛠️ FIX: Stripped legacy non-optimized framework dependencies to avoid include collision hazards
#include <ST7796_t3.h>

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

#endif
