#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <ST7796_t3.h>

// Structural button footprint for the transport bar layout
struct UI_Button {
  int x;
  int y;
  int w;
  int h;
  const char* label;
  uint16_t color;
  bool isPressed;
};

enum UIState {
  STATE_PLAYER,
  STATE_MENU
};

enum MenuLevel {
  LEVEL_ARTISTS,
  LEVEL_ALBUMS,
  LEVEL_TRACKS
};

enum EngineLifecycleState {
  ENGINE_IDLE,
  ENGINE_PRELOADING,
  ENGINE_STAGED,
  ENGINE_WAKING_UP,
  ENGINE_ACTIVE_PLAYING
};

// Global state machine tracking flags
extern UIState currentUIState;
extern MenuLevel currentMenuLevel;
extern int menuScrollOffset;

// 🚀 THE ISOLATED BROWSE REGISTER SHIELDS
extern int browseArtistIndex;
extern int browseAlbumIndex;

// Active playback memory trackers
extern int selectedArtistIndex;
extern int selectedAlbumIndex;

// Hardware graphics core mappings
extern ST7796_t3 tft;
extern uint16_t touchX;
extern uint16_t touchY;

// Color Tokens
#define COLOR_RAMS_BG 0x0841
#define COLOR_RAMS_CARD 0x18C3
#define COLOR_RAMS_DIVIDER 0x3186
#define COLOR_RAMS_WHITE 0xFFFF
#define COLOR_RAMS_ORANGE 0xFD00
#define COLOR_RAMS_TEXT_MUTE 0x7BEF

// 200x200 16-bit Artwork Cache Matrix Blocks
extern uint16_t activeArtworkCache[40000];
extern bool activeArtworkLoaded;

// Function Pipeline Exposure Signatures
void initDisplaySystem();
void refreshDisplayHardwareState();
void drawBootLoadingScreen();
void drawAudioDashboard();
void drawAlbumArtwork();
void updateTrackWindow(int trackNum, const char* trackTitle);
void handleLiveTimeAndProgressBar();
void processTouchControls();
void updatePlayPauseButtonLabel(const char* newLabel, uint16_t newColor);
void drawTransportButton(UI_Button btn);

void drawMenuScreen();
void drawArtistView();
void drawAlbumView();
void drawTrackView();
void processMenuTouch();
void processArtistViewTouch();
void processAlbumViewTouch();
void processTrackViewTouch();

void drawWrappedTextLine(const char* text, int startX, int startY, int maxW, int fontScale, uint16_t color, uint16_t bgColor, int lineSpacing, int maxLines, int& outNextY);
bool readTouchPanel(uint16_t& x, uint16_t& y);

#endif
