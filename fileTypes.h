#ifndef FILE_TYPES_GUARD
#define FILE_TYPES_GUARD

#include "readError.h"
#include "writeError.h"

/**
  Used to give a name to file type identifiers
*/
typedef enum {
  CS229,
  WAVE
} fileType_t;

/**
  Used to hold all the data associated with sounds (both CS229 and WAVE)
*/
typedef struct {
  unsigned long sampleRate;
  fileType_t fileType;
  char* fileName;
  void* rawData;
  unsigned int dataSize;
  readError_t error;
  unsigned short numChannels;
  unsigned short bitDepth;
} sound_t;

#endif
