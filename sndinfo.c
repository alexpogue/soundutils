#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileUtils.h"
#include "fileTypes.h"
#include "readError.h"
#include "errorPrinter.h"
#include <limits.h>

void printErrorsInSound(sound_t* sound);

/**
  Converts type to the corresponding string and puts the result into str
*/
void fileTypeToString(char* str, fileType_t type);

/**
  Print information from sound.
*/
void printSoundDetails(sound_t* sound);

/**
  Prints usage message.
*/
void printUsage(char* exeName);

int main(int argc, char* argv[]) {
  int i;
  if(argc < 2) {
    char* stdinFileName = "standard input file";
    sound_t* stdinSound = loadSound(stdin, stdinFileName);
    if(!stdinSound) {
      printMemoryError();
      exit(1);
    }
    if(stdinSound->error != NO_ERROR) {
      printErrorsInSound(stdinSound);
    }
    else {
      printSoundDetails(stdinSound);
    }
    unloadSound(stdinSound);
  }
  else {
    for(i = 1; i < argc; i++) {
      if('-' == argv[i][0]) {
        if('h' == argv[i][1]) {
          printUsage(argv[0]);
          exit(0);
        }
        else {
          printInvalidOptionError(argv[i][1]);
          exit(0);
        }
      }
    }
    for(i = 1; i < argc; i++) {
      char* fileName = argv[i];
      FILE* fp2 = fopen(fileName, "rb");
      sound_t* autoLoadedSound;
      printf("\n");
      if(!fp2) {
        printFileOpenError(fileName);
        exit(1);
      } 
      autoLoadedSound = loadSound(fp2, fileName);
      if(!autoLoadedSound) { 
        printMemoryError();
        exit(1);
      }
      if(autoLoadedSound->error != NO_ERROR) {
        printErrorsInSound(autoLoadedSound);
      }
      else {
        printSoundDetails(autoLoadedSound);
      }

      fclose(fp2);
      unloadSound(autoLoadedSound);
    }
  }
  printf("\n");
  exit(0);
}

void printErrorsInSound(sound_t* sound) {
  if(sound->error == ERROR_EOF) {
    printEofError();
  }
  else if(sound->error == ERROR_READING) {
    printFileReadError();
  }
  else if(sound->error == ERROR_MEMORY) {
    printMemoryError();
  }
  else if(sound->error == ERROR_FILETYPE) {
    printFileTypeError();
  }
  else if(sound->error == ERROR_BIT_DEPTH) {
    printBitDepthError();
  }
  else if(sound->error == ERROR_INVALID_KEYWORD) {
    printInvalidKeywordError();
  }
  else if(sound->error == ERROR_NO_VALUE) {
    printNoValueError();
  }
  else if(sound->error == ERROR_SAMPLE_DATA) {
    printSampleDataError();
  }
  else {
    printf("Unforeseen error occurred!\n");
  }
}

void printSoundDetails(sound_t* sound) {
  char typeStr[6];
  fileTypeToString(typeStr, sound->fileType);

  printf("File name: %s\n", sound->fileName);
  printf("File type: %s\n", typeStr);
  printf("Sample rate: %ld\n", sound->sampleRate);
  printf("Bit depth: %d\n", sound->bitDepth);
  printf("Number of channels: %d\n", sound->numChannels);
  printf("Number of samples: %d\n", calculateNumSamples(sound));
  printf("Sound length (seconds): %f\n", calculateSoundLength(sound));
}

void printUsage(char* exeName) {
  printf("Usage: %s [filename]\n", exeName);
}

void fileTypeToString(char* str, fileType_t type) {
  if(type == WAVE) {
    strcpy(str, "WAVE");
  }
  else if(type == CS229) {
    strcpy(str, "CS229");
  } 
}
