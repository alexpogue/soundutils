#ifndef FILE_UTILS_GUARD
#define FILE_UTILS_GUARD

#include <stdio.h>
#include "fileTypes.h"

sound_t* loadSound(FILE* file, char* fileName); 
sound_t* loadEmptySound(char* fileName);
void unloadSound(sound_t* sound);
void getFileType(FILE* file, sound_t* sound);
unsigned int calculateNumSamples(sound_t* sound);
float calculateSoundLength(sound_t* sound);

#endif
