#include <stdio.h>
#include <stdlib.h>
#include "errorPrinter.h"
#include "fileTypes.h"
#include "fileUtils.h"

/* TODO: FIX UNDEFINED REFERENCE ERROR */

void printHelp(char* cmd);
fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, long* outputChannel, char** outputFileName);

int main(int argc, char** argv) {
  fileType_t outputType;
  char isInputStdin, *outputFileName, **fileNames;
  int fileLimit, numFiles;
  long outputChannel;
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
  for(i = 0; i < numFiles; i++) {
    printf("filename %d: %s\n", i, fileNames[i]);
  }
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
  for(i = 0; i < numFiles; i++) {
    unloadSound(sounds[i]);
  }
  free(sounds);
  free(fileNames);
  return 0;
}

fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, long* outputChannel, char** outputFileName) {
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

