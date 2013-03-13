#ifndef FILE_UTILS_GUARD
#define FILE_UTILS_GUARD

#include <stdio.h>
#include "fileTypes.h"

sound_t* loadSound(FILE* file, char* fileName); 
sound_t* loadEmptySound();
void unloadSound(sound_t* sound);
void getFileType(FILE* file, sound_t* sound);
void cs229ToWave(sound_t* sound);
void waveToCs229(sound_t* sound);
void scaleBitDepth(int target, sound_t* sound);
unsigned int calculateNumSamples(sound_t* sound);
float calculateSoundLength(sound_t* sound);

#endif
