#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <ST7735_t3.h>
#include <ST7796_t3.h>

// --- DEFINED HERE FOR GLOBAL SCOPE ---
enum UIState {
  STATE_PLAYER, // "Now Playing" Dashboard
  STATE_MENU    // Library Navigation Views
};

struct UI_Button {
  int x, y, w, h;
  const char* label;
  uint16_t color;
  bool isPressed;
};

// Expose core hardware and coordinate instances globally across file divisions
extern ST7796_t3 tft;
extern UI_Button transport[];
extern uint16_t touchX;
extern uint16_t touchY;

// --- Subsystem Dashboard Control Prototyping ---
void initDisplaySystem();
void drawAudioDashboard();
void updateTrackWindow(int trackNum, const char* trackTitle);
void drawTransportButton(UI_Button btn);
void updatePlayPauseButtonLabel(const char* newLabel, uint16_t newColor);
void resetProgressTrackers();
void handleLiveTimeAndProgressBar();
void drawAlbumArtwork();
bool readTouchPanel(uint16_t &x, uint16_t &y);
void processTouchControls();

#endif
