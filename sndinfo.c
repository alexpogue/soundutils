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

/**
  Prints help screen.
*/
void printHelp(char* exeName);

int main(int argc, char* argv[]) {
  int i;
  if(argc < 2) {
    char* stdinFileName = "standard input file";
    sound_t* stdinSound = loadSound(stdin, stdinFileName);
    printf("\n");
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
          printHelp(argv[0]);
          exit(0);
        }
        else {
          printInvalidOptionError(argv[i][1]);
          exit(0);
        }
      }
    }
    for(i = 1; i < argc; i++) {
      sound_t* autoLoadedSound;
      char* fileName = argv[i];
      FILE* fp2 = fopen(fileName, "rb");
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
        printf("\n");
        printSoundDetails(autoLoadedSound);
      }

      fclose(fp2);
      unloadSound(autoLoadedSound);
    }
  }
  printf("\n");
  exit(0);
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
  printf("Sound length (seconds): %.3f\n", calculateSoundLength(sound));
}

void printUsage(char* exeName) {
  printf("Usage: %s [filename]\n", exeName);
}

void printHelp(char* exeName) {
  printf("Sndinfo Help: \n");
  printUsage(exeName);
  printf("This program reads each wav and CS229 sound file passed as\n");
  printf("arguments and outputs their information:\n");
  printf("Flags: \n");
  printf("-h\tdisplays this help page\n");
} 

void fileTypeToString(char* str, fileType_t type) {
  if(type == WAVE) {
    strcpy(str, "WAVE");
  }
  else if(type == CS229) {
    strcpy(str, "CS229");
  } 
}
