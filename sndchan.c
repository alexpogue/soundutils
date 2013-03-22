#include <stdio.h>
#include <stdlib.h>
#include "errorPrinter.h"
#include "fileTypes.h"
#include "fileUtils.h"

void printHelp(char* cmd);
fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, int* outputChannel, char** outputFileName);
void distributeIntoChannels(sound_t* dest, sound_t* src);
void combineChannels(sound_t* s1, sound_t* s2, fileType_t resultType);
void combineChannelsSoundArray(sound_t* dest, sound_t** sounds, int numSounds);

int main(int argc, char** argv) {
  fileType_t outputType;
  FILE* outputFile;
  char isInputStdin, *outputFileName, **fileNames;
  int fileLimit, numFiles;
  int outputChannel;
  sound_t *dest, **sounds;
  int i;
  isInputStdin = 0;
  outputFileName = NULL;
  numFiles = 0;
  outputChannel = -1;
  /* allocate enough space for every arg or 1 spot for stdin */
  fileLimit = argc;
  fileNames = malloc(sizeof(char*) * fileLimit);
  if(!fileNames) {
    printMemoryError();
    exit(1);
  }
  outputType = handleCommandLineArgs(argc, argv, fileNames, fileLimit, &numFiles, &outputChannel, &outputFileName);
  if(numFiles == -1) {
    /* means we printed help or invalid option */
    free(fileNames);
    exit(0);
  }
  if(numFiles == 0) {
    numFiles = 1;
    isInputStdin = 1;
  }
  sounds = malloc(sizeof(sound_t*) * numFiles);
  if(!sounds) {
    printMemoryError();
    free(fileNames);
    exit(1);
  }
  if(isInputStdin) {
    sounds[0] = loadSound(stdin, "StdinSound");
  }
  outputFile = fopen(outputFileName, "wb");
  if(!outputFile) {
    free(fileNames);
    free(sounds);
    printFileOpenError(outputFileName);
    exit(1);
  }

  for(i = 0; i < numFiles && !isInputStdin; i++) {
    FILE* fp;
    fp = fopen(fileNames[i], "rb");
    if(!fp) {
      printFileOpenError(fileNames[i]);
      free(sounds);
      free(fileNames);
      exit(1);
    }
    sounds[i] = loadSound(fp, fileNames[i]);
    fclose(fp);
  }
  dest = loadEmptySound();
  dest->fileType = outputType;
  combineChannelsSoundArray(dest, sounds, numFiles); 
  if(outputChannel > -1) {
    isolateChannel(dest, outputChannel);
  }
  writeSoundToFile(dest, outputFile, outputType);
  fclose(outputFile);
  unloadSound(dest);
  for(i = 0; i < numFiles; i++) {
    unloadSound(sounds[i]);
  }
  free(sounds);
  free(fileNames);
  return 0;
}

fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, int* outputChannel, char** outputFileName) {
  int i;
  /* will be reset to WAV if we see -w option */
  fileType_t outputType = CS229;
  /* -1 is value to output all channels */
  *outputChannel = -1;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      if(argv[i][1] == 'h') {
        printHelp(argv[0]);
        *numFilesRead = -1;
        return outputType;
      }
      else if(argv[i][1] == 'o') {
        *outputFileName = argv[i+1];
        /* don't reread the file name as an input file */
        ++i;
      }
      else if(argv[i][1] == 'w') {
        outputType = WAVE;
      }
      else if(argv[i][1] == 'c') {
        *outputChannel = strtol(argv[i+1], NULL, 10);
        /* don't include the number as a file name */
        ++i;
      }
      else {
        printInvalidOptionError(argv[i][1]);
        *numFilesRead = -1;
        return outputType;
      }
    }
    else {
      fileNames[(*numFilesRead)++] = argv[i];
    }
  }
  return outputType;
}

void combineChannelsSoundArray(sound_t* dest, sound_t** sounds, int numSounds) {
  int i;
  for(i = 0; i < numSounds; i++) {
    convertToFileType(dest->fileType, sounds[i]);
  }
  deepCopySound(dest, sounds[0]);
  for(i = 1; i < numSounds; i++) {
    combineChannels(dest, sounds[i], dest->fileType);
  }
}

void combineChannels(sound_t* s1, sound_t* s2, fileType_t resultType) {
  if(ensureSoundChannelsCombinable(s1, s2, resultType) == -1) {
    printSampleRateError();
    return;
  }
  distributeIntoChannels(s1, s2);
}

void distributeIntoChannels(sound_t* dest, sound_t* append) {
  int i, j, numSamples, bytesPerDestSample, bytesPerAppendSample;
  void* newData = realloc(dest->rawData, dest->dataSize);
  char *destCharData, *appendCharData;
  if(!newData) {
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->rawData = newData;
  appendCharData = (char*)append->rawData;
  /* add channels to make space for append's data */
  addZeroedChannels(append->numChannels, dest);
  destCharData = (char*)dest->rawData;
  numSamples = calculateNumSamples(dest);
  bytesPerDestSample = dest->numChannels * append->bitDepth / 8;
  bytesPerAppendSample = append->numChannels * append->bitDepth / 8;
  for(i = 0; i < numSamples; i++) {
    int firstNewData = bytesPerDestSample * (i + 1) - bytesPerAppendSample;
    for(j = 0; j < bytesPerAppendSample; j++) {
      destCharData[firstNewData + j] = appendCharData[i * bytesPerAppendSample + j];
    }
  }
}

void printHelp(char* cmd) {
  printf("Sndchan Help:\n");
  printf("Usage: %s file1 [, file2, ...] [options]\n\n", cmd);
  
  printf("Utility:\n");
  printf("This program reads the files passed as arguments and combines the channels of\n");
  printf("them into a single sound. The resulting sound will be the \"overlay\" of the\n");
  printf("source sounds. \n\n");
  
  printf("Defaults:\n");
  printf("Default output is in CS229 format (see -w option to change). \n");
  printf("Default output is to stdout unless another file is given (with -o)\n");
  printf("If no source files are given, tries to read file from stdin\n\n");

  printf("Options:\n");
  printf("-c [n]\t\tOnly include channel n in output (1st channel = 0, etc.)\n");
  printf("-h\t\tPrint this screen\n");
  printf("-o [fileName]\tOutput file to fileName\n");
  printf("-w\t\tOutput in WAVE format\n");
}

