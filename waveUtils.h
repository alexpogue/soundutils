#ifndef WAVE_UTILS_GUARD
#define WAVE_UTILS_GUARD

#include <stdio.h>
#include <stdlib.h>
#include "errorPrinter.h"
#include "fileTypes.h"

/* TODO: consider moving chunkId_t and wavData_t out of the header so that 
  other files don't try to access them. If we do this, we must also get rid of 
  all methods in this file that take wavData_t as a parameter and move them to 
  fileUtils.c. That means we must read wave files all at once and return a 
  derived sound_t. Or we could change the wavData_t parameters to tightly-
  coupled methods with sound_t parameters.
*/

/**
  Used to tell what chunk we are currently reading 
*/
typedef enum {
  CHUNK_UNKNOWN,
  CHUNK_FMT,
  CHUNK_DATA
} chunkId_t;

typedef struct {
  /** 
    data can point to the types: 
      (1) unsigned 8-bit integer if bitDepth == 8
      (2) signed 16-bit integer if bitDepth == 16
      (3) signed 32-bit integer if bitDepth == 32
  */
  void* data;
  chunkId_t currentChunkId;
  readError_t error;
  unsigned int numBytesInChunk;
  unsigned int sampleRate;
  unsigned int byteRate;
  unsigned int dataChunkSize;
  unsigned short audioFormat;
  unsigned short numChannels;
  unsigned short blockAlign;
  unsigned short bitDepth;
} wavData_t;

/** 
  Reads the wav file entirely into sound. If something goes wrong, error 
  message is printed and sound->error is set.
  Precondition: fp's file pointer is directly after the header
  Postcondition: fp has been completely read
*/
void wavRead(FILE* fp, sound_t* sound);

/**
  Reads the chunk id (ex. "fmt " or "data") and returns the corresponding 
  chunkId_t. If memory error occurs, error message is printed and 
  sound->error is set.
  Precondition: fp's file pointer is directly after file type specifier
  Postcondition: fp's file pointer is directly after chunk ID
*/
void wavReadChunkId(FILE* fp, wavData_t* wd);

/** 
  Reads the number of remaining bytes remaining in the current chunk. If 
  something goes wrong, error message is printed and sound->error is set. 
  Precondition: fp's file pointer is directly after chunk ID
  Postcondition: fp's file pointer is directly after num bytes in chunk speciier
*/
void wavReadNumRemainingBytesInChunk(FILE* fp, wavData_t* wd);

/** 
  Reads the audio format. If audio format is not 1 (for PCM), then error message
  is printed, and sound->error is set because other audio formats indicate 
  compression and we don't handle them.
  Precondition: fp's file pointer is directly after num bytes in chunk speciier
  Postcondition: fp's file pointer is directly after audio format.
*/
void wavReadAudioFormat(FILE* fp, wavData_t* wd);

/**
  Reads the number of channels in the sound. If something goes wrong, error 
  message is printed and sound->error is set.
  Precondition: fp's file pointer is directly after audio format
  Postcondition: fp's file pointer is directly after number of channels
*/ 
void wavReadNumChannels(FILE* fp, wavData_t* wd);

/**
  Reads the sample rate. If something goes wrong, error message is printed and 
  sound->error is set
  Precondition: fp's file pointer is directly after number of channels
  Postcondition: fp's file pointer is directly after sample rate.
*/
void wavReadSampleRate(FILE* fp, wavData_t* wd);

/**
  Read byte rate. If something goes wrong, error message is printed and 
  sound->error is set.
  Precondition: fp's file pointer is directly after sample rate.
  Postcondition: fp's file pointer is directly after byte rate.
*/
void wavReadByteRate(FILE* fp, wavData_t* wd);

/** 
  Read block align. If something goes wrong, error message is printed and 
  sound->error is set.
  Precondition: fp's file pointer is directly after byte rate.
  Postcondition: fp's file pointer is directly after block align.
*/

void wavReadBlockAlign(FILE* fp, wavData_t* wd);

/**
  Read bit depth. If something goes wrong, error message is printed and
  sound->error is set.
  Precondition: fp's file pointer is directly after block align.
  Postcondition: fp's file pointer is directly after bit depth.
*/
void wavReadBitDepth(FILE* fp, wavData_t* wd);

/**
  Writes wave file data from sound to fp.
*/
writeError_t writeWaveFile(sound_t* sound, FILE* fp);

#endif
