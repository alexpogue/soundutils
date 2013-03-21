#include "fileUtils.h"
#include "fileTypes.h"
#include "fileReader.h"
#include "waveUtils.h"
#include "cs229Utils.h"
#include "readError.h"
#include "writeError.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

void addDataToEndOfSound(sound_t* sound, unsigned int numData);
void addSample(sound_t* sound); 
unsigned int calculateTotalDataElements(sound_t* sound);

/** 
  Returns an allocated but empty sound_t*. Must manually call methods to 
  extract file data into the sound_t*. Returns NULL on memory error. 
*/
sound_t* loadEmptySound() {
  sound_t* sp = malloc(sizeof(sound_t));
  if(!sp) {
    return NULL;
  }
  sp->fileName = NULL;
  sp->error = NO_ERROR;
  sp->rawData = NULL;
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

  sp->fileName = malloc(strlen(fileName) + 1);
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
  if(sound->error != ERROR_MEMORY && sound->rawData != NULL && sound->dataSize != 0) {
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

void convertToFileType(fileType_t resultType, sound_t* sound) {
  if(resultType == CS229) {
    waveToCs229(sound);
  }
  else if(resultType == WAVE) {
    cs229ToWave(sound);
  }
}

void cs229ToWave(sound_t* sound) {
  signed char* charData = (signed char*)sound->rawData;
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
  char* cs229Data = (char*)sound->rawData;
  if(sound->fileType == CS229) {
    /* already correct type */
    return;
  }
  if(sound->bitDepth == 8) {
    /* waveData and cs229Data point to the same thing */
    unsigned char* waveData = (unsigned char*)sound->rawData;
    /* convert samples to signed */
    for(i = 0; i < calculateNumSamples(sound); i++) {
      cs229Data[i] = waveData[i] - 128;
    }
  }
  /* trim MIN_VALUE samples to MIN_VALUE + 1 (ex. -128 samples to -127, -32768 to -32767, etc.) */
  for(i = 0; i < calculateNumSamples(sound); i++) {
    if( (sound->bitDepth == 8 && cs229Data[i] == -128)
      || (sound->bitDepth == 16 && cs229Data[i] == -32768)
      || (sound->bitDepth == 32 && cs229Data[i] < -2147483647) ) {
      /* we used < -2147483647 because we need to use LL suffix for one less */
      cs229Data[i] += 1;
    }
  }
  sound->fileType = CS229;
}

int ensureSoundsCombinable(sound_t* s1, sound_t* s2) {
  if(s1->sampleRate != s2->sampleRate) {
    return -1;
  }
  ensureBitDepth(s1, s2);
  ensureNumChannels(s1, s2);
  return 0;
}

void ensureBitDepth(sound_t* s1, sound_t* s2) {
  if(s1->bitDepth > s2->bitDepth) {    
    scaleBitDepth(s1->bitDepth, s2);
  }
  else if(s1->bitDepth < s2->bitDepth) {
    scaleBitDepth(s2->bitDepth, s1);
  }
}

void ensureNumChannels(sound_t* s1, sound_t* s2) {
  if(s1->numChannels > s2->numChannels) {
    int numChannelsToAdd = s1->numChannels - s2->numChannels;
    addZeroedChannels(numChannelsToAdd, s2);
  }
  else if(s1->numChannels < s2->numChannels) {
    int numChannelsToAdd = s2->numChannels - s1->numChannels;
    addZeroedChannels(numChannelsToAdd, s1);
  }
}

void ensureChannelLength(sound_t* s1, sound_t* s2) {
  unsigned int numDataPerChannelS1 = calculateNumSamples(s1); 
  unsigned int numDataPerChannelS2 = calculateNumSamples(s2);
  if(numDataPerChannelS1 > numDataPerChannelS2) {
    unsigned int numDataToAdd = numDataPerChannelS1 - numDataPerChannelS2;
    addDataToEndOfSound(s2, numDataToAdd);
  }
  else if(numDataPerChannelS2 > numDataPerChannelS1) {
    unsigned int numDataToAdd = numDataPerChannelS2 - numDataPerChannelS1;
    addDataToEndOfSound(s1, numDataToAdd);
  }
}

void addDataToEndOfSound(sound_t* sound, unsigned int numData) {
  while(numData--) addSample(sound);
}

void addSample(sound_t* sound) {
  int i;
  unsigned int totalDataElements = calculateTotalDataElements(sound);
  if(sound->bitDepth == 8) {
    char* charData = (char*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      charData[totalDataElements + i] = 0;
    }
  }
  else if(sound->bitDepth == 16) {
    short* shortData = (short*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      shortData[totalDataElements + i] = 0;
    }
  }
  else if(sound->bitDepth == 32) {
    long* longData = (long*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      longData[totalDataElements + i] = 0;
    }
  }
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
  int i;
  int numDataElements = calculateTotalDataElements(sound);
  if(sound->bitDepth == 8) {
    char* charData = (char*)sound->rawData;
    for(i = 0; i < numDataElements; i++) {
      charData[i] *= sampleMultiplier;
    }
  }
  else if(sound->bitDepth == 16) {
    short* shortData = (short*)sound->rawData;
    for(i = 0; i < numDataElements; i++) {
      shortData[i] *= sampleMultiplier;
    }
  }
  else if(sound->bitDepth == 32) {
    long* longData = (long*)sound->rawData;
    for(i = 0; i < numDataElements; i++) {
      longData[i] *= sampleMultiplier;
    }
  }
  sound->bitDepth = target;
}

void addZeroedChannels(int howMany, sound_t* sound) {
  int i, j, k, newLastIndex;
  int newNumChannels = sound->numChannels + howMany;
  int numAdditionalData = howMany * calculateNumSamples(sound);
  int newSize = sound->dataSize + numAdditionalData * sound->bitDepth / 8;
  void* newData = realloc(sound->rawData, newSize);
  if(!newData) {
    sound->error = ERROR_MEMORY;
    return;
  }
  sound->rawData = newData;

  newLastIndex = calculateNumSamples(sound) * sound->numChannels + numAdditionalData - 1;
  j = calculateNumSamples(sound) * sound->numChannels;
  if(sound->bitDepth == 8 && sound->fileType == WAVE) {
    unsigned char* uCharData = (unsigned char*)sound->rawData;
    for(i = newLastIndex; i >= newNumChannels - 1; i-=(newNumChannels) ) {
      for(k = 0; k < howMany; k++) {
        uCharData[i-k] = 0;
      }
      for(k = i - howMany; k > i - newNumChannels; k--) {
        uCharData[k] = uCharData[--j];
      }
    }
  }
  else if(sound->bitDepth == 8 && sound->fileType == CS229) {
    signed char* charData = (signed char*)sound->rawData;
    for(i = newLastIndex; i >= newNumChannels - 1; i-=(newNumChannels) ) {
      for(k = 0; k < howMany; k++) {
        charData[i-k] = 0;
      }
      for(k = i - howMany; k > i - newNumChannels; k--) {
        charData[k] = charData[--j];
      }
    }
  }
  else if(sound->bitDepth == 16) {
    short* shortData = (short*)sound->rawData;
     for(i = newLastIndex; i >= newNumChannels - 1; i-=(newNumChannels) ) {
      for(k = 0; k < howMany; k++) {
        shortData[i-k] = 0;
      }
      for(k = i - howMany; k > i - newNumChannels; k--) {
        shortData[k] = shortData[--j];
      }
    }
  }
  else if(sound->bitDepth == 32) {
    long* longData = (long*)sound->rawData;
    for(i = newLastIndex; i >= newNumChannels - 1; i-=(newNumChannels) ) {
      for(k = 0; k < howMany; k++) {
        longData[i-k] = 0;
      }
      for(k = i - howMany; k > i - newNumChannels; k--) {
        longData[k] = longData[--j];
      }
    }
  }
  else {
    sound->error = ERROR_BIT_DEPTH;
  }
  sound->dataSize = newSize;
  sound->numChannels = newNumChannels;
} 

writeError_t writeSoundToFile(sound_t* sound, FILE* fp, fileType_t outputType) { 
  if(outputType == CS229) {
    writeCs229File(sound, fp);
  }
  else if(outputType == WAVE) {
    writeWaveFile(sound, fp);
  }
  return 0;
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

unsigned int calculateTotalDataElements(sound_t* sound) {
  return calculateNumSamples(sound) * sound->numChannels;
}

/*TODO: TEST */
void printData(sound_t* sound) {
  int i;
  if(sound->bitDepth == 8 && sound->fileType == CS229) {
    char* charData = (char*)sound->rawData;
    for(i = 0; i < calculateNumSamples(sound) * sound->numChannels; i++) {
      printf("%d:\t\t%d\n", i, charData[i]);
    }
  }
  else if(sound->bitDepth == 8 && sound->fileType == WAVE) {
    unsigned char* uCharData = (unsigned char*)sound->rawData;
    for(i = 0; i < calculateNumSamples(sound) * sound->numChannels; i++) {
      printf("%d:\t\t%d\n", i, uCharData[i]);
    }
  } 
  else if(sound->bitDepth == 16) {
    short* shortData = (short*)sound->rawData;
    for(i = 0; i < calculateNumSamples(sound) * sound->numChannels; i++) {
      printf("%d:\t\t%d\n", i, shortData[i]);
    }
  }
  else if(sound->bitDepth == 32) {
    long* longData = (long*)sound->rawData;
    for(i = 0; i < calculateNumSamples(sound) * sound->numChannels; i++) {
      printf("%d:\t\t%ld\n", i, longData[i]);
    }
  }
}
