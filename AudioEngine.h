#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include <Audio.h>

// Expose active nested directory path targets
extern const char* currentArtistFolder;
extern const char* currentAlbumFolder;

// Expose dynamic track structures (Fixed to proper multi-string signature bounds)
extern char trackQueue[30][64]; 
extern int totalTracks;
extern int currentTrackIndex;
extern bool isMediaPlaying;
extern bool isMediaPaused;
extern bool activeEngineIsA;
extern bool nextTrackPreLaunched;

// Expose core audio framework objects
extern AudioPlaySdWav playWav1;
extern AudioPlaySdWav playWav2;

// Functional audio engine commands
void initAudioSystem();
void scanCurrentAlbumFolder();
void updateAudioEngine();
void playFreshAlbumStart();
void preLoadNextTrack();

#endif
