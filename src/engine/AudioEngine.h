#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include <Audio.h>
#include <SD.h>
#include "../../src/ui/main/DisplayUI.h"  // Ensures EngineLifecycleState enum is visible globally

// Global Audio Configuration Footprints
#define PATH_BUFFER_SIZE 256
extern char currentArtistFolder[PATH_BUFFER_SIZE];
extern char currentAlbumFolder[PATH_BUFFER_SIZE];

// Dynamic Memory Track Queue allocations matching master data structures
extern char trackQueue[30][96];
extern int totalTracks;
extern int currentTrackIndex;

// Master Transport Control States
extern bool isMediaPlaying;
extern bool isMediaPaused;
extern bool activeEngineIsA;  // Retained to track layout orientation if needed

// Core Gateway Operational Directives
void initAudioSystem();
void playFreshAlbumStart();
void updateAudioEngine();
void preLoadNextTrack();  // Bypassed or modified under unified RAM pipeline

#endif
