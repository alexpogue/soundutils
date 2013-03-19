#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileUtils.h"
#include "errorPrinter.h"

void printHelp(char* cmd);
readError_t getErrorFromSounds(sound_t** sounds, int numSounds);
void handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, fileType_t* outputType, char** outputFileName);
void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType);
void concatenateData(sound_t* s1, sound_t* s2, sound_t* dest);

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
    if(i >= 1) {
      /*concatenateSounds(sounds[i-1], sounds[i], dest, CS229);*/
    }
  }
  free(fileNames);
  if(getErrorFromSounds(sounds, numFiles) == NO_ERROR) {
    dest = loadEmptySound();
    concatenateSounds(sounds[0], sounds[1], dest, CS229);

    if(outputFileName == NULL) {
      writeSoundToFile(dest, stdout);
    }
    else {
      FILE* fp;
      fp = fopen(outputFileName, "wb");
      if(!fp) {
        fprintf(stderr, "Could not open %s for writing\n", outputFileName);
      }
      writeSoundToFile(dest, fp);
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
  if(s1->bitDepth > s2->bitDepth) {    
    dest->bitDepth = s1->bitDepth;
    scaleBitDepth(s1->bitDepth, s2);
  }
  else if(s1->bitDepth < s2->bitDepth) {
    dest->bitDepth = s2->bitDepth;
    scaleBitDepth(s2->bitDepth, s1);
  }
  else {
    dest->bitDepth = s1->bitDepth;
  }
  if(s1->numChannels > s2->numChannels) {
    int numChannelsToAdd = s1->numChannels - s2->numChannels;
    dest->numChannels = s1->numChannels;
    addZeroedChannels(numChannelsToAdd, s2);
  }
  else if(s1->numChannels < s2->numChannels) {
    int numChannelsToAdd = s2->numChannels - s1->numChannels;
    dest->numChannels = s2->numChannels;
    addZeroedChannels(numChannelsToAdd, s1);
  }
  else {
    dest->numChannels = s1->numChannels;
  }
  concatenateData(s1, s2, dest);
  dest->sampleRate = s1->sampleRate;
  dest->fileType = resultType;
}

void concatenateData(sound_t* s1, sound_t* s2, sound_t* dest) {
  int i;
  char *s1CharData, *s2CharData, *destCharData;
  int newDataSize = s1->dataSize + s2->dataSize;
  /* we can access these as chars because we are only writing them */
  char* newData = (char*)realloc(dest->rawData, newDataSize);
  if(!newData) {
    printMemoryError();
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->rawData = newData;
  if(s1->fileType != s2->fileType 
    || s1->sampleRate != s2->sampleRate 
    || s1->bitDepth != s2->bitDepth
    || s1->numChannels != s2->numChannels) {
    /* TODO: REMOVE THIS TEST PRINT */
    printf("We cannot cat these two sounds!\n");
  }

  s1CharData = (char*)s1->rawData;
  s2CharData = (char*)s2->rawData;
  destCharData = (char*)dest->rawData;
  for(i = 0; i < s1->dataSize; i++) {
    destCharData[i] = s1CharData[i];
  }
  for(i = 0; i < s2->dataSize; i++) {
    destCharData[i + s1->dataSize] = s2CharData[i];
  }
  dest->dataSize = newDataSize;
}
