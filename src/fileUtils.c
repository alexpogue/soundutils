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

void addSamplesToEndOfSound(sound_t* sound, unsigned int numData);
void addSample(sound_t* sound, unsigned int sampleIndex); 
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

int ensureSoundChannelsCombinable(sound_t* s1, sound_t* s2, fileType_t resultType) {
  if(ensureSoundsCombinable(s1, s2, resultType) == -1) {
    return -1;
  }
  ensureChannelLength(s1, s2);
  return 0;
}

int ensureSoundsCanConcatenate(sound_t* s1, sound_t* s2, fileType_t resultType) {
  if(ensureSoundsCombinable(s1, s2, resultType) == -1) {
    return -1;
  }
  ensureNumChannels(s1, s2);
  return 0;
}

int ensureSoundsMixable(sound_t* s1, sound_t* s2, fileType_t resultType) {
  if(ensureSoundsCombinable(s1, s2, resultType) == -1) {
    return -1;
  }
  ensureNumChannels(s1, s2);
  ensureChannelLength(s1, s2);
  return 0;
}

int ensureSoundsCombinable(sound_t* s1, sound_t* s2, fileType_t resultType) {
  if(s1->sampleRate != s2->sampleRate) {
    return -1;
  }
  convertToFileType(resultType, s1);
  convertToFileType(resultType, s2);
  ensureBitDepth(s1, s2);
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
    addSamplesToEndOfSound(s2, numDataToAdd);
  }
  else if(numDataPerChannelS2 > numDataPerChannelS1) {
    unsigned int numDataToAdd = numDataPerChannelS2 - numDataPerChannelS1;
    addSamplesToEndOfSound(s1, numDataToAdd);
  }
}

void addSamplesToEndOfSound(sound_t* sound, unsigned int numSamples) {
  void* newData;
  unsigned int addedDataSize = numSamples * sound->numChannels * sound->bitDepth / 8;
  sound->dataSize += addedDataSize;
  newData = realloc(sound->rawData, sound->dataSize);
  if(!newData) {
    sound->error = ERROR_MEMORY;
  }
  sound->rawData = newData;
  while(numSamples) {
    addSample(sound, calculateNumSamples(sound) - numSamples--);
  }
}

/*TODO: fix this */
void addSample(sound_t* sound, unsigned int sampleIndex) {
  int i;
  if(sound->bitDepth == 8 && sound->fileType == CS229) {
    char* charData = (char*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      charData[sampleIndex * sound->numChannels + i] = 0;
    }
  }
  else if(sound->bitDepth == 8 && sound->fileType == WAVE) {
    unsigned char* uCharData = (unsigned char*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      uCharData[sampleIndex * sound->numChannels + i] = 127;
    }
  }
  else if(sound->bitDepth == 16) {
    short* shortData = (short*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      shortData[sampleIndex * sound->numChannels + i] = 0;
    }
  }
  else if(sound->bitDepth == 32) {
    long* longData = (long*)sound->rawData;
    for(i = 0; i < sound->numChannels; i++) {
      longData[sampleIndex * sound->numChannels + i] = 0;
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

void convertToBitsPerData(int bitsPerData, sound_t* sound ) {
  unsigned int numDataElements = calculateTotalDataElements(sound);
  if(sound->bitDepth == bitsPerData) {
    return;
  }
  if(sound->bitDepth > bitsPerData) {
    /* TODO: REMOVE TEST PRINT */
    printf("Programmer: you tried to convert bitDepth down.");
    return;
  }
  if(sound->bitDepth == 8) {
    char* charData = (char*)sound->rawData;
    if(bitsPerData == 16) {
      short* shortData = malloc(sizeof(short) * numDataElements);
      while(numDataElements--) {
        shortData[numDataElements] = (short)charData[numDataElements];
      }
      sound->rawData = shortData;
    }
    else if(bitsPerData == 32) {
      long* longData = malloc(sizeof(long) * numDataElements);
      while(numDataElements--) {
        longData[numDataElements] = (long)charData[numDataElements];
      }
      sound->rawData = longData;
      free(charData);
    }
  }
  else if(sound->bitDepth == 16) {
    short* shortData = (short*)sound->rawData;
    if(bitsPerData == 32) {
      long* longData = malloc(sizeof(long) * numDataElements);
      while(numDataElements--) {
        longData[numDataElements] = (long)shortData[numDataElements];
      }
      sound->rawData = longData;
      free(shortData);
    }
  }
  sound->dataSize *= bitsPerData / sound->bitDepth;
  sound->bitDepth *= bitsPerData / sound->bitDepth;
}

void scaleBitDepth(int target, sound_t* sound) {
  float sampleMultiplier = ipow(2, target) / ipow(2, sound->bitDepth);
  int i;
  unsigned int numDataElements = calculateTotalDataElements(sound);
  convertToBitsPerData(target, sound);
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
        uCharData[i-k] = 127;
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

void isolateChannel(sound_t* sound, unsigned int channelNum) {
  int i, j, newDataSize;
  void* newData;
  char* charData = (char*)sound->rawData;
  int bytesPerData = sound->bitDepth / 8;
  int numSamples = calculateNumSamples(sound);
  if(sound->numChannels == 1) {
    return;
  }
  for(i = 0; i < numSamples; i++) {
    for(j = 0; j < bytesPerData; j++) {
      charData[i * bytesPerData + j] = charData[(i * sound->numChannels + channelNum) * bytesPerData + j];
    }
  }
  newDataSize = numSamples * bytesPerData;
  newData = realloc(sound->rawData, newDataSize);
  if(!newData) {
    sound->error = ERROR_MEMORY;
    return;
  }
  sound->rawData = newData;
  sound->dataSize = newDataSize;
  sound->numChannels = 1;
}

void deepCopySound(sound_t* dest, sound_t* src) {
  int i;
  char *destCharData, *srcCharData, *newFileName;
  void* newData;
  srcCharData = (char*)src->rawData;
  dest->sampleRate = src->sampleRate;
  dest->fileType = src->fileType;
  newFileName = realloc(dest->fileName, strlen(src->fileName) + 1);
  if(!newFileName) {
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->fileName = newFileName;
  strcpy(dest->fileName, src->fileName);
  newData = realloc(dest->rawData, src->dataSize);
  if(!newData) {
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->rawData = newData;
  destCharData = (char*)dest->rawData;
  for(i = 0; i < src->dataSize; i++) {
    destCharData[i] = srcCharData[i];
  }
  dest->dataSize = src->dataSize;
  dest->error = src->error;
  dest->numChannels = src->numChannels;
  dest->bitDepth = src->bitDepth;
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

readError_t getErrorFromSounds(sound_t** sounds, int numSounds) {
  while(numSounds) {
    if(sounds[--numSounds]->error != NO_ERROR) {
      return sounds[numSounds]->error;
    }
  }
  return NO_ERROR;
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
