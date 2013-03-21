#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileUtils.h"
#include "errorPrinter.h"

void printHelp(char* cmd);
readError_t getErrorFromSounds(sound_t** sounds, int numSounds);
fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, char** outputFileName);
void concatenateSoundArray(sound_t* dest, sound_t** sounds, int numSounds);
void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType);
void deepCopySound(sound_t* dest, sound_t* src);
void concatenateData(sound_t* dest, sound_t* append);

int main(int argc, char** argv) {
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
  fileNames = malloc(sizeof(char*) * argc);
  if(!fileNames) {
    printMemoryError();
    exit(1);
  }
  outputType = handleCommandLineArgs(argc, argv, fileNames, fileLimit, &numFiles, &outputFileName);
  /* numFiles of -1 means we printed help or had an invalid option */
  if(numFiles == -1) {
    exit(0);
  }
  if(numFiles == 0) {
    /* for single-file stdin file input */
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
      fclose(fp);
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

fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int capacity, int* numFilesRead, char** outputFileName) {
  int i;
  /* will be reset to WAV if we see -w option */
  fileType_t outputType = CS229;
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
  printf("Sndcat Help:\n");
  printf("Usage: %s file1 [, file2, ...] [options]\n\n", cmd);

  printf("Utility:\n");
  printf("This program reads the CS229/WAVE file(s) passed as arguments, concatenates them,\n");
  printf("and writes the result to a file (default:stdout , see -o option). Outputs in the\n");
  printf("requested format (default:CS229, see -w option)\n\n");

  printf("Defaults:\n");
  printf("Default output file type is CS229 (see -w option)\n");
  printf("Default output file is stdout unless another file is provided with -o.\n");
  printf("If no source file(s) are provided by the arguments, input is read from stdin.\n");
  printf("If only one source file is provided, the file is rewritten as-is in the desired\n"); 
  printf("format. (useful for converting between formats)\n\n");

  printf("Options:\n");
  printf("-h\t\tdisplays this help page\n");
  printf("-o [file]\toutput to a file rather than standard out\n");
  printf("-w\t\toutput in the WAVE format rather than CS229\n");
}

/**
  Concatenate numSounds sounds from the sound_t* array and put the resulting
  sound into dest. 
*/
void concatenateSoundArray(sound_t* dest, sound_t** sounds, int numSounds) {
  int i;
  fileType_t oldType;
  convertToFileType(dest->fileType, sounds[0]);
  /* "un"copy the file type of sounds[0] */
  oldType = dest->fileType;
  deepCopySound(dest, sounds[0]);
  dest->fileType = oldType;
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
void deepCopySound(sound_t* dest, sound_t* src) {
  int i;
  char *destCharData, *srcCharData, *newFileName;
  void* newData;
  srcCharData = (char*)src->rawData;
  dest->sampleRate = src->sampleRate;
  dest->fileType = src->fileType;
  newFileName = realloc(dest->fileName, strlen(src->fileName) + 1);
  if(!newFileName) {
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->fileName = newFileName;
  strcpy(dest->fileName, src->fileName);
  newData = realloc(dest->rawData, src->dataSize);
  if(!newData) {
    dest->error = ERROR_MEMORY;
    return;
  }
  dest->rawData = newData;
  destCharData = (char*)dest->rawData;
  for(i = 0; i < src->dataSize; i++) {
    destCharData[i] = srcCharData[i];
  }
  dest->dataSize = src->dataSize;
  dest->error = src->error;
  dest->numChannels = src->numChannels;
  dest->bitDepth = src->bitDepth;
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

  concatenateData(s1, s2);
  dest->sampleRate = s1->sampleRate;
  dest->fileType = resultType;
}

void concatenateData(sound_t* dest, sound_t* append) {
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
