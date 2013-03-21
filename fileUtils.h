#ifndef FILE_UTILS_GUARD
#define FILE_UTILS_GUARD

#include <stdio.h>
#include "fileTypes.h"
#include "writeError.h"

sound_t* loadSound(FILE* file, char* fileName); 
sound_t* loadEmptySound();
void unloadSound(sound_t* sound);
void getFileType(FILE* file, sound_t* sound);
void cs229ToWave(sound_t* sound);
void waveToCs229(sound_t* sound);
void ensureBitDepth(sound_t* s1, sound_t* s2);
void ensureNumChannels(sound_t* s1, sound_t* s2);
void scaleBitDepth(int target, sound_t* sound);
void addZeroedChannels(int howMany, sound_t* sound);
unsigned int calculateNumSamples(sound_t* sound);
float calculateSoundLength(sound_t* sound);
writeError_t writeSoundToFile(sound_t* sound, FILE* fp, fileType_t outputType);

/* TODO: TEST */
void printData(sound_t* sound);

#endif
