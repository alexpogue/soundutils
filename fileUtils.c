#include "fileUtils.h"
#include "fileTypes.h"
#include "fileReader.h"
#include "waveUtils.h"
#include "cs229Utils.h"
#include "readError.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/** 
  Returns an allocated but empty sound_t*. Must manually call methods to 
  extract file data into the sound_t*. Returns NULL on memory error. 
*/
sound_t* loadEmptySound() {
  sound_t* sp = (sound_t*) malloc(sizeof(sound_t));
  if(!sp) {
    return NULL;
  }
  sp->error = NO_ERROR;
  sp->dataSize = 0;
  return sp;
}

/** 
  Loads whole sound into sound_t* and returns it. Sets sound->error and returns 
  sound on error. Returns NULL on memory allocation error. 
*/
sound_t* loadSound(FILE* file, char* fileName) {
  sound_t* sp = loadEmptySound();
  if(!sp) {
    return NULL;
  }

  sp->fileName = (char*)malloc(strlen(fileName + 1));
  if(!sp->fileName) {
    free(sp);
    return NULL;
  }
  strcpy(sp->fileName, fileName);

  getFileType(file, sp);
  if(sp->error != NO_ERROR) {
    return sp;
  }
  if(WAVE == sp->fileType) {
    wavRead(file, sp);
  }
  if(CS229 == sp->fileType) {
    cs229Read(file, sp);
  }
  return sp;
}

void unloadSound(sound_t* sound) {
  if(sound->error != ERROR_MEMORY && sound->rawData != NULL && sound->dataSize == 0) {
    free(sound->rawData);
  }
  if(sound->fileName != NULL) {
    free(sound->fileName);
  }
  free(sound);
}

/*
  Retrieve the file type from first few bytes of the file. Fills in 
  sound->fileType with the proper fileType_t value. Prints memory error and 
  read error, no other errors printed.
  Precondition: file pointer is at beginning of file
  Postcondition: file pointer directly after file specifier
*/
void getFileType(FILE* file, sound_t* sound) {
  char type[5];
  if(sound->error == NO_ERROR) {
    sound->error = readBytes(type, 4, file);
  }
  if(sound->error == NO_ERROR && strncmp(type, "RIFF", 4) == 0) {
    /* so far looks like a wave file */
    /* read past 4 "filesize" bytes, following 4 bytes should be "WAVE" */
    sound->error = readBytes(type, 4, file);
    if(sound->error == NO_ERROR) {
      sound->error = readBytes(type, 4, file);
    }
    if(sound->error == NO_ERROR && strncmp(type, "WAVE", 4) != 0) {
      sound->error = ERROR_FILETYPE;
    }
    sound->fileType = WAVE;
  }
  else if(sound->error == NO_ERROR) {
    /* so far, we've read "CS22", now read the "9" */
    /* readBytes instead of fgetc because readBytes handles read errors */
    sound->error = readBytes(&type[4], 1, file);
    toLowerCase(type, 5);
    if(sound->error == NO_ERROR && strncmp(type, "cs229", 5) == 0) {
      sound->fileType = CS229;
    }
    else {
      /* the header is not "WAVE" nor "cs229". Couldn't identify filetype */
      sound->error = ERROR_FILETYPE;
    }
  }
  /* trim MIN_VALUE samples to MIN_VALUE + 1 (ex. -128 samples to -127, -32768 to -32767, etc.) */
  return;
}

void cs229ToWave(sound_t* sound) {
  char* charData = (char*)sound->rawData;
  if(sound->fileType == WAVE) {
    /* already correct type */
    return;
  }
  if(sound->bitDepth == 8) {
    /* convert to unsigned, cs229 allows -127 to 127, wav (bit depth of 8) allows 0-255 */
    int i;
    for(i = 0; i < calculateNumSamples(sound); i++) {
      charData[i] += 128;
    }
  }
  sound->fileType = WAVE;
}

void waveToCs229(sound_t* sound) {
  int i;
  signed char* charData = (signed char*)sound->rawData;
  if(sound->fileType == CS229) {
    /* already correct type */
    return;
  }
  if(sound->bitDepth == 8) {
    /* convert samples to signed */
    for(i = 0; i < calculateNumSamples(sound); i++) {
      charData[i] -= 128;
    }
  }
  /* trim MIN_VALUE samples to MIN_VALUE + 1 (ex. -128 samples to -127, -32768 to -32767, etc.) */
  for(i = 0; i < calculateNumSamples(sound); i++) {
    if( (sound->bitDepth == 8 && charData[i] == -128)
      || (sound->bitDepth == 16 && charData[i] == -32768)
      || (sound->bitDepth == 32 && charData[i] < -2147483647) ) {
      charData[i] += 1;
    }
  }
  sound->fileType = CS229;
}

float ipow(int base, int exp) {
  int i; 
  float origBase, fBase;
  fBase = origBase = (float)base;
  if(exp < 0 ) {
    return 0;
  }
  if(exp == 0) {
    return 1;
  }
  for(i = 0; i < exp; i++) {
    fBase *= origBase;
  }
  return fBase;
}

void scaleBitDepth(int target, sound_t* sound) {
  float sampleMultiplier = ipow(2, target) / ipow(2, sound->bitDepth);
  sound->bitDepth *= sampleMultiplier;
}

unsigned int calculateNumSamples(sound_t* sound) {
  if(sound->error != NO_ERROR 
      || sound->numChannels == 0 
      || sound->bitDepth == 0) {
    return 0;
  }
  return (sound->dataSize * 8) / (sound->numChannels * sound->bitDepth);
}

unsigned int calculateBlockAlign(sound_t* sound) {
  if(sound->error != NO_ERROR) return 0;
  return sound->bitDepth / 8 * sound->numChannels;
}

unsigned int calculateByteRate(sound_t* sound) {
  if(sound->error != NO_ERROR) return 0;
  return sound->sampleRate * calculateBlockAlign(sound);
}

float calculateSoundLength(sound_t* sound) {
  if(sound->error != NO_ERROR || calculateByteRate(sound) == 0) return 0;
  return (float)sound->dataSize / calculateByteRate(sound);
}
