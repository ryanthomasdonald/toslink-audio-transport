#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryCommon.h"
#include <SD.h>

// Standard paths retain their boundary tracking definitions
char currentArtistFolder[PATH_BUFFER_SIZE] = "";
char currentAlbumFolder[PATH_BUFFER_SIZE] = "";

extern char currentAlbumAbsolutePath[256];

// Instantiations matching our master header parameters exactly
char trackQueue[30][96];
int totalTracks = 0;
int currentTrackIndex = 0;
bool isMediaPlaying = false;
bool isMediaPaused = false;
bool activeEngineIsA = true;
bool nextTrackPreLaunched = false;

EngineLifecycleState activeEngineState = ENGINE_IDLE;

AudioPlaySdWav playWav1;
AudioPlaySdWav playWav2;
AudioMixer4 audioMixerL;
AudioMixer4 audioMixerR;
AudioOutputSPDIF3 spdif1;

AudioConnection patchCord1(playWav1, 0, audioMixerL, 0);
AudioConnection patchCord2(playWav2, 0, audioMixerL, 1);
AudioConnection patchCord3(audioMixerL, 0, spdif1, 0);
AudioConnection patchCord4(playWav1, 1, audioMixerR, 0);
AudioConnection patchCord5(playWav2, 1, audioMixerR, 1);
AudioConnection patchCord6(audioMixerR, 0, spdif1, 1);

void initAudioSystem() {
  AudioMemory(48);
  audioMixerL.gain(0, 1.0);
  audioMixerL.gain(1, 0.0);
  audioMixerR.gain(0, 1.0);
  audioMixerR.gain(1, 0.0);
}

void playFreshAlbumStart() {
  playWav1.stop();
  playWav2.stop();
  delay(10);

  // Combine the absolute folder path with our solid pre-sorted RAM queue filename
  char fullTrackExecutionPath[384];
  snprintf(fullTrackExecutionPath, sizeof(fullTrackExecutionPath), "%s%s",
           currentAlbumAbsolutePath, trackQueue[currentTrackIndex]);

  Serial.printf("AUDIO ENGINE: Stream initialized targeting: %s\n", fullTrackExecutionPath);

  // Synchronize text string caches for our display views
  if (selectedArtistIndex >= 0 && selectedAlbumIndex >= 0) {
    if (library[selectedArtistIndex].name != NULL) {
      snprintf(currentArtistFolder, sizeof(currentArtistFolder), "%s", library[selectedArtistIndex].name);
    }
    if (library[selectedArtistIndex].albums[selectedAlbumIndex].name != NULL) {
      snprintf(currentAlbumFolder, sizeof(currentAlbumFolder), "%s", library[selectedArtistIndex].albums[selectedAlbumIndex].name);
    }
  }

  AudioNoInterrupts();
  if (activeEngineIsA) {
    playWav1.play(fullTrackExecutionPath);
  } else {
    playWav2.play(fullTrackExecutionPath);
  }
  AudioInterrupts();

  isMediaPlaying = true;
  isMediaPaused = false;
  nextTrackPreLaunched = false;

  // 🚀 THE INTERLOCK PRIME: Arm the wake-up guard to ensure data edges settle safely
  activeEngineState = ENGINE_WAKING_UP;
}

void preLoadNextTrack() {
  int nextTrackIndex = currentTrackIndex + 1;
  if (nextTrackIndex >= totalTracks) return;

  // Assemble the absolute folder track path for the preloaded audio stream
  char fullTrackPreloadPath[384];
  snprintf(fullTrackPreloadPath, sizeof(fullTrackPreloadPath), "%s%s",
           currentAlbumAbsolutePath, trackQueue[nextTrackIndex]);

  Serial.printf("AUDIO ENGINE: Preloading next track from absolute path: %s\n", fullTrackPreloadPath);

  // 🚀 STAGE PRELOAD PHASE
  activeEngineState = ENGINE_PRELOADING;

  if (activeEngineIsA) {
    if (playWav2.play(fullTrackPreloadPath)) {
      delay(5);
      playWav2.togglePlayPause();
    }
  } else {
    if (playWav1.play(fullTrackPreloadPath)) {
      delay(5);
      playWav1.togglePlayPause();
    }
  }
}

void updateAudioEngine() {
  if (!isMediaPlaying) return;
  int nextTrackIndex = currentTrackIndex + 1;

  // =========================================================================
  // 🚀 PHASE 1: STAGED HARDWARE DEBOUNCE MONITORING
  // Protects the system against high-capacity 512GB SD cluster lookup lag
  // =========================================================================
  if (activeEngineState == ENGINE_WAKING_UP) {
    // Look at the active hardware channel and wait until it is TRULY playing
    bool hardwareConfirmedPlaying = activeEngineIsA ? playWav1.isPlaying() : playWav2.isPlaying();
    if (hardwareConfirmedPlaying) {
      activeEngineState = ENGINE_ACTIVE_PLAYING;
      Serial.println("AUDIO ENGINE: Target channel confirmed active. Interlock released.");
    }
    return;  // Absolute lock: freeze checking track expirations until file opens
  }

  // Handle background preloading lifecycle stages cleanly
  if (!nextTrackPreLaunched && nextTrackIndex < totalTracks) {
    preLoadNextTrack();
    nextTrackPreLaunched = true;
  }

  // Catch when the preloaded track successfully caches into memory staging
  if (activeEngineState == ENGINE_PRELOADING) {
    bool nextChannelLoaded = activeEngineIsA ? playWav2.isPlaying() : playWav1.isPlaying();
    // If the hardware reports false, it means togglePlayPause caught the file and paused it cleanly
    if (!nextChannelLoaded) {
      activeEngineState = ENGINE_STAGED;
      Serial.println("AUDIO ENGINE: Next file staging sector cached cleanly in RAM.");
    }
  }

  // =========================================================================
  // 🚀 PHASE 2: SAFE NATURAL CROSSOVER MONITORING
  // =========================================================================
  if (activeEngineState == ENGINE_STAGED || activeEngineState == ENGINE_ACTIVE_PLAYING) {
    bool currentChannelFinished = activeEngineIsA ? !playWav1.isPlaying() : !playWav2.isPlaying();

    if (currentChannelFinished) {
      if (nextTrackIndex < totalTracks) {
        AudioNoInterrupts();
        if (activeEngineIsA) {
          audioMixerL.gain(0, 0.0);
          audioMixerL.gain(1, 1.0);
          audioMixerR.gain(0, 0.0);
          audioMixerR.gain(1, 1.0);
          playWav2.togglePlayPause();  // Wake up Engine B
          activeEngineIsA = false;
        } else {
          audioMixerL.gain(0, 1.0);
          audioMixerL.gain(1, 0.0);
          audioMixerR.gain(0, 1.0);
          audioMixerR.gain(1, 0.0);
          playWav1.togglePlayPause();  // Wake up Engine A
          activeEngineIsA = true;
        }
        AudioInterrupts();

        currentTrackIndex++;
        nextTrackPreLaunched = false;

        // 🚀 THE INTERLOCK SHIELD: Slam the state back to waking up!
        // This completely prevents double skips during high-latency SD read frames.
        activeEngineState = ENGINE_WAKING_UP;

        updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
        Serial.printf("AUDIO ENGINE: Crossover execution advanced to track %d\n", currentTrackIndex);
      } else {
        isMediaPlaying = false;
        activeEngineState = ENGINE_IDLE;
        updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
        updateTrackWindow(currentTrackIndex + 1, "ALBUM FINISHED");
      }
    }
  }
}
