#include "waveUtils.h"
#include "fileReader.h"
#include "readError.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/** 
  Read the whole format chunk, and put the data into wd. Sets wd->error and
  returns when error occurs. 
  Precondition: file pointer directly after "fmt "
  Postcondition: file pointer directly after the fmt chunk
*/
void wavReadFmtChunk(FILE* fp, wavData_t* wd);

/**
  Read the whole data chunk and put data into wd->data. Sets wd->error and 
  returns when error occurs.
  Precondition: file poitner directly after "data"
  Postcondition: file pointer directly after the data chunk
*/ 
void wavReadDataChunk(FILE* fp, wavData_t* wd);

/** 
  Read through the chunk and ignore the data inside. Sets wd->error and returns
  when an error occurs.
  Preconditon: file poiner directly before numBytesInChunk specifier
  Postcondition: file poiner directly after chunk 
*/
void wavIgnoreChunk(FILE* fp, wavData_t* wd);

/**
  Ignore num bytes from fp.
*/
void wavIgnoreBytes(FILE* fp, size_t num);

/**
  Fills in sound_t* with the data from the waveData_t*, including error field
*/
void wavToSound(wavData_t* wd, sound_t* sound);

/**
  Finds the given chunkId in the file and calls its read function. 
  REMEMBER, fmt chunk ALWAYS comes first in wave files.
*/
void wavFindAndReadChunk(FILE* fp, wavData_t* wd, chunkId_t cId);

/**
  Read sample data from the data chunk and put it in the correct place in 
  wd->data based on bitDepth.
*/
void wavReadSoundData(FILE* fp, wavData_t* wd);

void wavRead(FILE* fp, sound_t* sound) {
  wavData_t* wData = (wavData_t*) malloc(sizeof(wavData_t));
  if(!wData) {
    sound->error = ERROR_MEMORY;
    return;
  }

  wData->dataChunkSize = 0;

  wavFindAndReadChunk(fp, wData, CHUNK_FMT);
  if(wData->error != NO_ERROR) {
    sound->error = wData->error;
    free(wData);
    return;
  }
  wavFindAndReadChunk(fp, wData, CHUNK_DATA);
  if(wData->error != NO_ERROR) {
    sound->error = wData->error;
  }
  wavToSound(wData, sound);
  free(wData);
}

void wavFindAndReadChunk(FILE* fp, wavData_t* wd, chunkId_t cId) {
  if(wd->error != NO_ERROR) return;
  wavReadChunkId(fp, wd);
  if(wd->error != NO_ERROR) return;
  while(wd->error == NO_ERROR && wd->currentChunkId != cId) {
    wavIgnoreChunk(fp, wd);
    if(wd->error != NO_ERROR) return;
    wavReadChunkId(fp, wd);
    if(wd->error != NO_ERROR) return;
  }
  if(wd->error == NO_ERROR) {
    switch(cId) {
      case CHUNK_FMT: 
        wavReadFmtChunk(fp, wd);
        break;
      case CHUNK_DATA:
        wavReadDataChunk(fp, wd);
        break;
      default:
        break;
    }
  }
}

void wavToSound(wavData_t* wd, sound_t* sound) {
  sound->sampleRate = wd->sampleRate;
  sound->bitDepth = wd->bitDepth;
  sound->numChannels = wd->numChannels;
  sound->dataSize = wd->dataChunkSize;
  sound->error = wd->error;
}   

void wavReadFmtChunk(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wavReadNumRemainingBytesInChunk(fp, wd);
  if(wd->error != NO_ERROR) return;
  wavReadAudioFormat(fp, wd);
  if(wd->error != NO_ERROR) return;
  wavReadNumChannels(fp, wd);
  if(wd->error != NO_ERROR) return;
  wavReadSampleRate(fp, wd);
  if(wd->error != NO_ERROR) return;
  wavReadByteRate(fp, wd);
  if(wd->error != NO_ERROR) return;
  wavReadBlockAlign(fp, wd);
  if(wd->error != NO_ERROR) return;
  wavReadBitDepth(fp, wd);
  if(wd->bitDepth != 8 && wd->bitDepth != 16 && wd->bitDepth != 32) {
    wd->error = ERROR_BIT_DEPTH;
  }
  /* TODO: ensure bit depth is a proper value (8,16,32) Adventurous? support 24 */

  /* ignore fmt chunk's extra parameters (numBytes - 16 previously read bytes */
  /* we make sure numBytesInChunk is >16 so we don't try ignoring negative bytes!*/
  if(wd->numBytesInChunk > 16) wavIgnoreBytes(fp, wd->numBytesInChunk - 16);
}

void wavReadDataChunk(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wavReadNumRemainingBytesInChunk(fp, wd);
  if(wd->error != NO_ERROR) return;
  if(wd->bitDepth == 8 && wd->numBytesInChunk > 0) {
    wd->data = (unsigned char*) malloc(wd->numBytesInChunk);
  }
  else if(wd->bitDepth == 16 && wd->numBytesInChunk > 0) {
    wd->data = (signed short*) malloc(wd->numBytesInChunk);
  }
  else if(wd->bitDepth == 32 && wd->numBytesInChunk > 0) {
    wd->data = (signed int*) malloc(wd->numBytesInChunk);
  }
  if((wd->bitDepth == 8 && !wd->data) || 
    (wd->bitDepth == 16 && !wd->data) ||
    (wd->bitDepth == 32 && !wd->data) ) 
  {
    wd->error = ERROR_MEMORY;
    return;
  }
  wd->dataChunkSize = wd->numBytesInChunk; 
  wavReadSoundData(fp, wd);
}

void wavReadSoundData(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(wd->data, wd->dataChunkSize, fp);
}
  
void wavIgnoreChunk(FILE* fp, wavData_t* wd) {
  size_t chunkSize;
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&chunkSize, 4, fp);
  if(wd->error != NO_ERROR) return;
  wavIgnoreBytes(fp, chunkSize);
}

void wavIgnoreBytes(FILE* fp, size_t num) {
  char ignoredChar;
  while(num-- && readBytes(&ignoredChar, 1, fp) == NO_ERROR); 
}

void wavReadChunkId(FILE* fp, wavData_t* wd) {
  char chunkStr[4];
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(chunkStr, 4, fp);
  if(wd->error != NO_ERROR) return; 
  if(strncmp(chunkStr, "fmt ", 4) == 0) {
    wd->currentChunkId = CHUNK_FMT;
  }
  else if(strncmp(chunkStr, "data", 4) == 0) {
    wd->currentChunkId = CHUNK_DATA;
  }
  else {
    wd->currentChunkId = CHUNK_UNKNOWN;
  }
}

void wavReadNumRemainingBytesInChunk(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&wd->numBytesInChunk, 4, fp);
}

void wavReadAudioFormat(FILE* fp, wavData_t* wd) {
 if(wd->error != NO_ERROR) return;
 wd->error = readBytes(&wd->audioFormat, 2, fp);
}

void wavReadNumChannels(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&wd->numChannels, 2, fp);
}

void wavReadSampleRate(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&wd->sampleRate, 4, fp);
}

void wavReadByteRate(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&wd->byteRate, 4, fp);
}

void wavReadBlockAlign(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&wd->blockAlign, 2, fp);
}

void wavReadBitDepth(FILE* fp, wavData_t* wd) {
  if(wd->error != NO_ERROR) return;
  wd->error = readBytes(&wd->bitDepth, 2, fp);
}
