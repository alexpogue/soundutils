#ifndef FILE_TYPES_GUARD
#define FILE_TYPES_GUARD

#include "readError.h"
#include "writeError.h"
/* TODO: CONSIDER MOVING TO fileUtils.h */
typedef enum {
  CS229,
  WAVE
} fileType_t;

typedef struct {
  unsigned long sampleRate;
  fileType_t fileType;
  char* fileName;
  void* rawData;
  unsigned int dataSize;
  readError_t error;
  /* TODO: Change these to chars, max numChannels is 128, max bitdepth is 32 */
  unsigned short numChannels;
  unsigned short bitDepth;
} sound_t;

#endif
