#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryCommon.h"
#include <SD.h>

// 🚀 DIRECT LINK TO UNIFIED GLOBALS RESIDENT INSIDE THE MAIN .INO FILE
extern volatile uint32_t ringWritePointer;
extern volatile uint32_t ringReadPointer;
extern volatile bool bankNeedRefill;
extern volatile int activeRefillBank;

extern EngineLifecycleState activeEngineState;
extern bool nextTrackPreLaunched;

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

// 🚀 Instantiate parallel queues to handle true synchronized Stereo!
AudioPlayQueue queueLeft;
AudioPlayQueue queueRight;
AudioOutputSPDIF3 spdifOut;

// Link separate queues directly to S/PDIF input slots 0 and 1
AudioConnection patchCord1(queueLeft, 0, spdifOut, 0);
AudioConnection patchCord2(queueRight, 0, spdifOut, 1);

// 🚀 Allocate our massive 8MB Circular Ring Buffer directly into external PSRAM space
#define BANK_SIZE_BYTES (1024 * 1024 * 4)
#define TOTAL_BUFFER_SIZE (BANK_SIZE_BYTES * 2)
EXTMEM char circularAudioBuffer[TOTAL_BUFFER_SIZE];

// Micro-slice tracking milestones
uint32_t currentBankWriteProgressBytes = 0;
#define REFILL_SLICE_SIZE_BYTES 16384      // 16KB lightweight chunk limits
const uint32_t audioDataStartOffset = 44;  // EAC standard uncompressed PCM offset

File activeAudioFile;

void initAudioSystem() {
  AudioMemory(120);
}

// Helper function to dynamically open the next track from our RAM library pointer queue map
bool advanceToNextPlaylistFile() {
  if (activeAudioFile) activeAudioFile.close();

  currentTrackIndex++;
  if (currentTrackIndex >= totalTracks) {
    Serial.println("🏁 AUDIO ENGINE: Reached final album track index boundary.");
    return false;  // Album fully loaded into memory
  }

  // Dynamically reconstruct the absolute file path targeting the next indexed track asset
  char fullTrackExecutionPath[384];
  snprintf(fullTrackExecutionPath, sizeof(fullTrackExecutionPath), "%s/%s",
           currentAlbumAbsolutePath, trackQueue[currentTrackIndex]);

  activeAudioFile = SD.open(fullTrackExecutionPath);
  if (!activeAudioFile) {
    Serial.printf("❌ FILE ERROR: Failed to open path: %s\n", fullTrackExecutionPath);
    return false;
  }

  activeAudioFile.seek(audioDataStartOffset);
  Serial.printf("🔀 SEAMLESS HANDOFF: Opened next indexed track asset: %s\n", trackQueue[currentTrackIndex]);
  return true;
}

void playFreshAlbumStart() {
  if (activeAudioFile) activeAudioFile.close();

  // Reconstruct absolute pathing strings with correct path separators
  char fullTrackExecutionPath[384];
  snprintf(fullTrackExecutionPath, sizeof(fullTrackExecutionPath), "%s/%s",
           currentAlbumAbsolutePath, trackQueue[currentTrackIndex]);

  Serial.printf("\n📥 AUDIO ENGINE: Initializing fresh manual launch pass for path: %s\n", fullTrackExecutionPath);

  // Synchronize text string caches for our display views
  if (selectedArtistIndex >= 0 && selectedAlbumIndex >= 0) {
    if (library[selectedArtistIndex].name != NULL) {
      snprintf(currentArtistFolder, sizeof(currentArtistFolder), "%s", library[selectedArtistIndex].name);
    }
    if (library[selectedArtistIndex].albums[selectedAlbumIndex].name != NULL) {
      snprintf(currentAlbumFolder, sizeof(currentAlbumFolder), "%s", library[selectedArtistIndex].albums[selectedAlbumIndex].name);
    }
  }

  activeAudioFile = SD.open(fullTrackExecutionPath);
  if (!activeAudioFile) {
    Serial.println("❌ FILE ERROR: Cannot access target track path.");
    return;
  }
  activeAudioFile.seek(audioDataStartOffset);

  // Completely prime the 8MB ring buffer immediately from the starting file
  uint32_t totalBytesWritten = 0;
  bool localPrimingFinished = false;

  while (totalBytesWritten < TOTAL_BUFFER_SIZE && !localPrimingFinished) {
    uint32_t spaceRemaining = TOTAL_BUFFER_SIZE - totalBytesWritten;
    uint32_t safeReadSize = (spaceRemaining > 4096) ? 4096 : (spaceRemaining - (spaceRemaining % 4));
    if (safeReadSize == 0) break;

    int bytesRead = activeAudioFile.read((uint8_t*)&circularAudioBuffer[totalBytesWritten], safeReadSize);
    if (bytesRead > 0) {
      uint32_t alignedBytes = bytesRead - (bytesRead % 4);
      totalBytesWritten += alignedBytes;
      if (bytesRead != (int)alignedBytes) {
        activeAudioFile.seek(activeAudioFile.position() - (bytesRead - alignedBytes));
      }
    } else {
      if (!advanceToNextPlaylistFile()) {
        localPrimingFinished = true;
        break;
      }
    }
  }

  ringWritePointer = totalBytesWritten % TOTAL_BUFFER_SIZE;
  ringReadPointer = 0;
  bankNeedRefill = false;
  activeRefillBank = 0;
  currentBankWriteProgressBytes = 0;

  isMediaPlaying = true;
  isMediaPaused = false;
  nextTrackPreLaunched = false;

  activeEngineState = ENGINE_ACTIVE_PLAYING;
  updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
  Serial.printf("AUDIO ENGINE: Priming complete. Cached %lu continuous bytes.\n", totalBytesWritten);
}

// 🚀 THE ASYNCHRONOUS MICRO-SLICE BACKGROUND LOADER
void processBackgroundSDFill() {
  if (!bankNeedRefill) return;

  uint32_t bankStartOffset = activeRefillBank * BANK_SIZE_BYTES;
  uint32_t sliceWriteAddress = bankStartOffset + currentBankWriteProgressBytes;
  uint32_t bankSpaceRemaining = BANK_SIZE_BYTES - currentBankWriteProgressBytes;

  uint32_t readTargetSize = (bankSpaceRemaining > REFILL_SLICE_SIZE_BYTES) ? REFILL_SLICE_SIZE_BYTES : bankSpaceRemaining;

  int bytesRead = activeAudioFile.read((uint8_t*)&circularAudioBuffer[sliceWriteAddress], readTargetSize);

  if (bytesRead > 0) {
    uint32_t alignedBytes = bytesRead - (bytesRead % 4);
    currentBankWriteProgressBytes += alignedBytes;

    if (bytesRead != (int)alignedBytes) {
      activeAudioFile.seek(activeAudioFile.position() - (bytesRead - alignedBytes));
    }
  } else {
    if (!advanceToNextPlaylistFile()) {
      nextTrackPreLaunched = true;
      bankNeedRefill = false;
      return;
    }
  }

  if (currentBankWriteProgressBytes >= BANK_SIZE_BYTES) {
    ringWritePointer = (bankStartOffset + currentBankWriteProgressBytes) % TOTAL_BUFFER_SIZE;
    currentBankWriteProgressBytes = 0;
    bankNeedRefill = false;
    Serial.printf("AUDIO ENGINE: 4MB Bank %d packed via micro-slices. Active Track: %s\n",
                  activeRefillBank, trackQueue[currentTrackIndex]);
  }
}

// 🚀 THE MAIN OPERATIONAL LOOP PASSTHROUGH GATEWAY
void updateAudioEngine() {
  if (!isMediaPlaying) return;

  // Check if the album has fully completed playback down to the very final sample byte
  if (nextTrackPreLaunched && ringReadPointer == ringWritePointer) {
    isMediaPlaying = false;
    activeEngineState = ENGINE_IDLE;
    if (activeAudioFile) activeAudioFile.close();
    updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
    updateTrackWindow(currentTrackIndex + 1, "ALBUM FINISHED");
    Serial.println("AUDIO ENGINE: Full album stream completed cleanly.");
    return;
  }

  // Standard 128-sample stereo blocks require exactly 512 bytes of raw data.
  if (queueLeft.available() && queueRight.available()) {
    int16_t* dmaBufferSlotL = queueLeft.getBuffer();
    int16_t* dmaBufferSlotR = queueRight.getBuffer();

    if (dmaBufferSlotL != NULL && dmaBufferSlotR != NULL) {
      uint32_t* stereoFrameSource = (uint32_t*)&circularAudioBuffer[ringReadPointer];

      // 🚀 THE VERIFIED LITTLE-ENDIAN ALIGNMENT MATRIX:
      // Low 16-bits hold the Left channel, High 16-bits hold Right!
      for (int i = 0; i < 128; i++) {
        uint32_t packedWord = stereoFrameSource[i];
        dmaBufferSlotL[i] = (int16_t)(packedWord & 0xFFFF);
        dmaBufferSlotR[i] = (int16_t)(packedWord >> 16);
      }

      queueLeft.playBuffer();
      queueRight.playBuffer();

      static int lastTrackIndexTracked = -1;
      if (currentTrackIndex != lastTrackIndexTracked) {
        lastTrackIndexTracked = currentTrackIndex;
        updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
      }

      ringReadPointer = (ringReadPointer + 512) % TOTAL_BUFFER_SIZE;

      if (ringReadPointer == BANK_SIZE_BYTES && !bankNeedRefill) {
        activeRefillBank = 0;
        currentBankWriteProgressBytes = 0;
        bankNeedRefill = true;
      } else if (ringReadPointer == 0 && !bankNeedRefill) {
        activeRefillBank = 1;
        currentBankWriteProgressBytes = 0;
        bankNeedRefill = true;
      }
    }
  }
  processBackgroundSDFill();
}
