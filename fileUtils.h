#ifndef FILE_UTILS_GUARD
#define FILE_UTILS_GUARD

#include <stdio.h>
#include "fileTypes.h"
#include "writeError.h"

/** 
  Automatically load sound by allocating memory for a sound, filling in each
  data field with the appropriate data from file, and returning the sound. must
  later call unloadSound to free the data.
*/
sound_t* loadSound(FILE* file, char* fileName); 

/**
  Loads a sound which only has space allocated with some default values. Fill 
  these in by calling CS229 or WAVE reading functions by hand or deepCopy'ing
  another sound to this one. Must call unloadSound to free data.
*/
sound_t* loadEmptySound();

/**
  Frees data associated with sound. Sound must have been allocated by either
  loadEmptySound or loadSound.
*/
void unloadSound(sound_t* sound);

/**
  Reads the first few bytes of the file (either "RIFF####WAVE" or "CS229") and
  extracts the file type from it. It then sets sound->fileType to the 
  appropriate fileType_t
*/
void getFileType(FILE* file, sound_t* sound);

/** 
  Convert file to another file type. If file is already the correct type it does
  not do anything.
*/
void convertToFileType(fileType_t resultType, sound_t* sound);

/**
  Convert CS229 formatted sound to WAVE. Mainly, we just have to adjust sample
  data limits.
*/
void cs229ToWave(sound_t* sound);

/**
  Convert WAVE formatted sound to CS229. Mainly, we just have to adjust sample
  data limits.
*/
void waveToCs229(sound_t* sound);

/**
  Ensures bitDepths and numChannels are the same, converts them to resultType.  
  Returns 0 on success and -1 if sampleRates are not the same.
*/
int ensureSoundsCanConcatenate(sound_t* s1, sound_t* s2, fileType_t resultType);

/**
  Matches up the bitDepth of s1 and s2 and converts both to resultType. Returns
  0 on success, and -1 if the sampleRates are not the same.
*/ 
int ensureSoundsCombinable(sound_t* s1, sound_t* s2, fileType_t resultType);

/**
  Ensures bitDepths, numChannels, and NumSamples are the same, converts them to
  result type. Returns 0 on success and -1 if sampleRates are not the same.
*/
int ensureSoundsMixable(sound_t* s1, sound_t* s2, fileType_t resultType);

/**
  Ensures bitDepths and channel length are the same, converts them to resultType.  
  Returns 0 on success and -1 if sampleRates are not the same.
*/
int ensureSoundChannelsCombinable(sound_t* s1, sound_t* s2, fileType_t resultType);

/**
  Scales bitDepth of the sound (s1 or s2) with lesser bitDepth than the other.
*/
void ensureBitDepth(sound_t* s1, sound_t* s2);

/**
  Adds zeroed-out channels to the sound (s1 or s2) with fewer channels until 
  the number of channels are equal.
*/
void ensureNumChannels(sound_t* s1, sound_t* s2);

/**
  Adds samples to the sound with lesser samples until the two numSamples are
  equal.
*/
void ensureChannelLength(sound_t* s1, sound_t* s2);

/**
  Scales the bitDepth of sound to target. Assumes that target is a supported
  bitDepth.
*/
void scaleBitDepth(int target, sound_t* sound);

/** 
  Adds howMany zeroed out channels to sound.
*/
void addZeroedChannels(int howMany, sound_t* sound);

/**
  Isolates the channelNum in sound. The sound will only have one channel after
  this method call.
*/
void isolateChannel(sound_t* sound, unsigned int channelNum);

/**
  Copy the members of src to sound pointed to by dest.
*/
void deepCopySound(sound_t* dest, sound_t* src);

/**
  Retrieves the error data element from each sound and returns the first 
  readError_t != NO_ERROR
*/
readError_t getErrorFromSounds(sound_t** sounds, int numSounds);

/**
  Uses dataSize, bitDepth, and numChannels to calculate the number of samples 
  in the given sound.
*/
unsigned int calculateNumSamples(sound_t* sound);

/**
  Calculates the sound length in seconds
*/
float calculateSoundLength(sound_t* sound);

/**
  Calculates total data elements in sound sample data.
*/
unsigned int calculateTotalDataElements(sound_t* sound);

/**
  Writes the sound to the file fp (opened for writing, otherwise we'll throw
  an error), and in the given outputType.
*/
writeError_t writeSoundToFile(sound_t* sound, FILE* fp, fileType_t outputType);

/** 
  A test function to print the data values contained in the given sound.
*/
void printData(sound_t* sound);

#endif
