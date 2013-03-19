#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileUtils.h"
#include "errorPrinter.h"

void printHelp(char* cmd);
readError_t getErrorFromSounds(sound_t** sounds, int numSounds);
void handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, fileType_t* outputType, char** outputFileName);
void concatenateSoundArray(sound_t* dest, sound_t** sounds, int numSounds);
void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType);
void deepCopySound(sound_t* dest, sound_t src);
void ensureBitDepth(sound_t* s1, sound_t* s2);
void ensureNumChannels(sound_t* s1, sound_t* s2);
void concatenateData(sound_t* s1, sound_t* s2, sound_t* dest);
void concatenateData2(sound_t* dest, sound_t* append);

int main(int argc, char** argv) {
  /* TODO: FIX FOR STDIN */
  int i, fileLimit, numFiles;
  char **fileNames, *outputFileName, isInputStdin;
  sound_t *dest, **sounds;
  fileType_t outputType;
  sounds = NULL;
  fileNames = NULL;
  outputFileName = NULL;
  numFiles = 0;
  isInputStdin = 0;
  /* allocate enough space for every arg or 1 spot for stdin */
  fileLimit = argc;
  fileNames = (char**)malloc(sizeof(char*) * argc);
  if(!fileNames) {
    printMemoryError();
    exit(1);
  }
  handleCommandLineArgs(argc, argv, fileNames, fileLimit, &numFiles, &outputType, &outputFileName);
  /* numFiles of -1 means we printed help or had an invalid option */
  if(numFiles == -1) {
    exit(0);
  }
  if(numFiles == 0) {
    /* for single-file stdin file input */
    numFiles = 1;
    isInputStdin = 1;
  }
  sounds = (sound_t**)malloc(sizeof(sound_t*) * numFiles);
  if(!sounds) {
    printMemoryError();
    free(fileNames);
    exit(1);
  }
  if(isInputStdin) {
    sounds[0] = loadSound(stdin, "StdinSound");
  }
  for(i = 0; i < numFiles && !isInputStdin; i++) {
    FILE* fp;
    fp = fopen(fileNames[i], "rb");
    if(!fp) {
      printFileOpenError(fileNames[i]);
      free(fileNames);
      free(sounds);
      exit(1);
    }
    sounds[i] = loadSound(fp, fileNames[i]);
    fclose(fp);
  }
  free(fileNames);
  if(getErrorFromSounds(sounds, numFiles) == NO_ERROR) {
    dest = loadEmptySound();
    dest->fileType = outputType;
    concatenateSoundArray(dest, sounds, numFiles);

    if(outputFileName == NULL) {
      writeSoundToFile(dest, stdout, outputType);
    }
    else {
      FILE* fp;
      fp = fopen(outputFileName, "wb");
      if(!fp) {
        printFileOpenError(outputFileName);
        free(sounds);
        exit(1);
      }
      writeSoundToFile(dest, fp, outputType);
    }
  
    unloadSound(dest);
  }
  else {
    for(i = 0; i < numFiles; i++) {
      printErrorsInSound(sounds[i]);
    }
  }
  for(i = 0; i < numFiles; i++) {
    unloadSound(sounds[i]);
  }
  free(sounds);
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

void handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, fileType_t* outputType, char** outputFileName) {
  int i;
  /* will be reset to WAV if we see -w option */
  *outputType = CS229;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      if(argv[i][1] == 'h') {
        printHelp(argv[0]);
        *numFilesRead = -1;
        return;
      }
      else if(argv[i][1] == 'o') {
        *outputFileName = argv[i+1];
        /* don't reread the file name as an input file */
        ++i;
      }
      else if(argv[i][1] == 'w') {
        *outputType = WAVE;
      }
      else {
        printInvalidOptionError(argv[i][1]);
        *numFilesRead = -1;
        return;
      }
    }
    else {
      fileNames[(*numFilesRead)++] = argv[i];
    }
  }
}

void printHelp(char* cmd) {
  printf("Sndcat Help:\n");
  printf("Usage %s file1 [, file2, ...] [options]\n", cmd);
  printf("This program reads the file(s) passed as arguments, concatenates them, and writes\n");
  printf("it to a file (defaults to stdout)\n");
  printf("Options:\n");
  printf("-h\t\tdisplays this help page\n");
  printf("-o [file]\t\toutput to a file rather than standard out\n");
}
 
/** 
  Convert file to another file type. If file is already the correct type it does
  not do anything.
*/
void convertToFileType(fileType_t resultType, sound_t* sound) {
  if(resultType == CS229) {
    waveToCs229(sound);
  }
  else if(resultType == WAVE) {
    cs229ToWave(sound);
  }
}

/**
  Concatenate numSounds sounds from the sound_t* array and put the resulting
  sound into dest. Changes the value of sounds[0];
*/
void concatenateSoundArray(sound_t* dest, sound_t** sounds, int numSounds) {
  int i;
  deepCopySound(dest, *sounds[0]);
  for(i = 0; i < numSounds; i++) {
    convertToFileType(dest->fileType, sounds[i]);
  }
  for(i = 1; i < numSounds; i++) {
    concatenateSounds(dest, sounds[i], dest, dest->fileType);
  }
}

/**
  Copy the members of src to sound pointed to by dest.
*/
void deepCopySound(sound_t* dest, sound_t src) {
  int i;
  char* srcCharData = (char*)src.rawData;
  char** destCharData = (char**)&(dest->rawData);
  void* newData = realloc(dest->rawData, src.dataSize);
  dest->sampleRate = src.sampleRate;
  dest->fileType = src.fileType;
  dest->fileName = (char*)malloc(strlen(src.fileName) + 1);
  if(!dest->fileName) {
    dest->error = ERROR_MEMORY;
    return;
  }
  strcpy(dest->fileName, src.fileName);
  
  if(!newData) {
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->rawData = newData;
  for(i = 0; i < src.dataSize; i++) {
    (*destCharData)[i] = srcCharData[i];
  }
  dest->dataSize = src.dataSize;
  dest->error = src.error;
  dest->numChannels = src.numChannels;
  dest->bitDepth = src.bitDepth;
}

/**
  concatenate sounds and store the concatenated sound into dest. Use the
  format specified by resultType 
*/
void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType) {
  convertToFileType(resultType, s1);
  convertToFileType(resultType, s2);
  if(s1->sampleRate != s2->sampleRate) {
    fprintf(stderr, "Incompatible sample rate error\n");
    return;
  }
  ensureBitDepth(s1, s2);
  dest->bitDepth = s1->bitDepth;
  ensureNumChannels(s1, s2);
  dest->numChannels = s1->numChannels;

  concatenateData2(s1, s2);
  dest->sampleRate = s1->sampleRate;
  dest->fileType = resultType;
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

void concatenateData2(sound_t* dest, sound_t* append) {
  int i;
  char *destCharData, *appendCharData;
  int newDataSize = dest->dataSize + append->dataSize;
  /* we can access these as chars because we are only writing them */
  char* newData = (char*)realloc(dest->rawData, newDataSize);
  if(!newData) {
    printMemoryError();
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->rawData = newData;
  if(dest->fileType != append->fileType 
    || dest->sampleRate != append->sampleRate 
    || dest->bitDepth != append->bitDepth
    || dest->numChannels != append->numChannels) {
    /* TODO: REMOVE THIS TEST PRINT */
    printf("We cannot cat these two sounds!\n");
  }
  destCharData = (char*)dest->rawData;
  appendCharData = (char*)append->rawData;
  for(i = 0; i < append->dataSize; i++) {
    destCharData[dest->dataSize + i] = appendCharData[i];
  }
  dest->dataSize = newDataSize;
}
