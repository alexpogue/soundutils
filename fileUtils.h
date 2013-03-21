#ifndef FILE_UTILS_GUARD
#define FILE_UTILS_GUARD

#include <stdio.h>
#include "fileTypes.h"
#include "writeError.h"

sound_t* loadSound(FILE* file, char* fileName); 
sound_t* loadEmptySound();
void unloadSound(sound_t* sound);
void getFileType(FILE* file, sound_t* sound);
/** 
  Convert file to another file type. If file is already the correct type it does
  not do anything.
*/
void convertToFileType(fileType_t resultType, sound_t* sound);
void cs229ToWave(sound_t* sound);
void waveToCs229(sound_t* sound);
/**
  Matches up the bitDepth and numChannels of s1 and s2. Returns 0 on success, 
  and -1 if the sampleRates are not the same.
*/ 
int ensureSoundsCombinable(sound_t* s1, sound_t* s2);
void ensureBitDepth(sound_t* s1, sound_t* s2);
void ensureNumChannels(sound_t* s1, sound_t* s2);
void ensureChannelLength(sound_t* s1, sound_t* s2);
void scaleBitDepth(int target, sound_t* sound);
void addZeroedChannels(int howMany, sound_t* sound);
unsigned int calculateNumSamples(sound_t* sound);
float calculateSoundLength(sound_t* sound);
writeError_t writeSoundToFile(sound_t* sound, FILE* fp, fileType_t outputType);

/* TODO: TEST */
void printData(sound_t* sound);

#endif
