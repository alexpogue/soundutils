#include <stdio.h>
#include <stdlib.h>
#include "fileTypes.h"
#include "fileUtils.h"
#include "errorPrinter.h"

/**
  Fills in filenames, numFilesRead, outputFileName, scalarStrs, and 
  numScalarsRead from the command line arguments. Returns the requested
  output fileType_t
*/
fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int* numFilesRead, char** outputFileName, char** scalarStrs, int* numScalarsRead);

/**
  Mixes sounds together by scaling each sample by their scalar, adding the 
  sounds' sample data together mathematically, and placing the resulting file
  into dest.
*/
void mixSounds(sound_t* dest, sound_t** sounds, float* scalars, int numSounds);

/**
  Converts the string array strings to floats and place the result into floats.
  Only read up to numData, and return 0 if we encounter an error, otherwise
  return 1;
*/
char stringsToFloats(char** strings, float* floats, unsigned int numData);

/**
  Scales the sample data of sound by the given scalar.
*/
void scaleSampleData(sound_t* sound, float scalar);

/**
  Scales numChars chars from char array by the given scalar.
*/
void scaleChars(char* chars, int numChars, float scalar);

/**
  Scales numShorts shorts from short array by the given scalar.
*/
void scaleShorts(short* shorts, int numShorts, float scalar);

/**
  Scales numLongs longs from long array by the given scalar.
*/
void scaleLongs(long* longs, int numLongs, float scalar);

/**
  Mathematically adds the sample data of the the two sounds (dest and addend)
  and stores the result in dest. This function allows overflow and is expected
  to receive values that will not overflow
*/
void addSampleData(sound_t* dest, sound_t* addend);

/**
  Prints message when we receive too many files by command line.
*/
void printTooManyFilesError();

/**
  Prints message when we receive too many scalars by command line.
*/
void printTooManyScalarsError();

/**
  Prints error message when we cannot successfully convert command line 
  arguments to scalar floats.
*/
void printScalarConversionError();

/**
  Prints the usage of this utility without any newlines.
*/
void printUsage(char* cmd);

/**
  Prints fully-formatted help screen and displays on stdout.
*/
void printHelp(char* cmd);

int main(int argc, char** argv) {
  int i, numFiles, numScalars;
  char *outputFileName, **fileNames, **scalarStrs;
  sound_t *dest, **sounds;
  float* scalarFloats;
  FILE* outputFile;
  fileType_t outputType;
  outputFileName = NULL;
  scalarStrs = NULL;
  scalarFloats = NULL;
  numFiles = 0;
  numScalars = 0;
  /* make enough room for all files */
  fileNames = malloc(sizeof(char*) * argc);
  if(!fileNames) {
    printMemoryError();
    exit(1);
  }
  scalarStrs = malloc(sizeof(int) * argc);
  if(!scalarStrs) {
    printMemoryError();
    free(fileNames);
    exit(1);
  }
  outputType = handleCommandLineArgs(argc, argv, fileNames, &numFiles, &outputFileName, scalarStrs, &numScalars);
  if(numFiles == -1) {
    /* we printed help or encountered an invalid option */ 
    free(fileNames);
    free(scalarStrs);
    exit(0);
  }
  if(numFiles == 0) {
    printUsage(argv[0]);
    printf("\n");
    exit(0);
  }
  if(numFiles > numScalars) {
    printTooManyFilesError();
  }
  else if(numFiles < numScalars) {
    printTooManyScalarsError();
    free(fileNames);
    free(scalarStrs);
    exit(1);
  }
  scalarFloats = malloc(sizeof(float) * numScalars);
  if(!scalarFloats) {
    printMemoryError();
    free(fileNames);
    free(scalarStrs);
    exit(1);
  }
  if(!stringsToFloats(scalarStrs, scalarFloats, numScalars)) {
    /* Error message printed in stringsToFloats */
    free(fileNames);
    free(scalarStrs);
    exit(1);
  }
  free(scalarStrs);
  sounds = malloc(numFiles * sizeof(sound_t*));
  if(!sounds) {
    printMemoryError();
    free(fileNames);
    exit(1);
  }
  if(outputFileName == NULL) {
    outputFile = stdout;
  }
  else {
    outputFile = fopen(outputFileName, "wb");
    if(!outputFile) {
      free(fileNames);
      free(sounds);
      printFileOpenError(outputFileName);
      exit(1);
    }
  }

  for(i = 0; i < numFiles; i++) {
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
  free(fileNames);
  if(getErrorFromSounds(sounds, numFiles) != NO_ERROR) {
    for(i = 0; i < numFiles; i++) {
      printErrorsInSound(sounds[i]);
    }
    free(sounds);
    exit(1);
  }
  dest = loadEmptySound();
  dest->fileType = outputType; 
  if(!dest) {
    printMemoryError();
    free(fileNames);
    exit(1);
  }
  mixSounds(dest, sounds, scalarFloats, numFiles);
  if(outputFileName == NULL) {
    writeSoundToFile(dest, stdout, outputType);
  }
  else {
    FILE* fp;
    fp = fopen(outputFileName, "wb");
    if(!fp) {
      printFileOpenError(outputFileName);
      free(sounds);
      unloadSound(dest);
      fclose(fp);
      exit(1);
    }
    writeSoundToFile(dest, fp, outputType);
    fclose(fp);
  } 
  unloadSound(dest);
  for(i = 0; i < numFiles; i++) {
    unloadSound(sounds[i]);
  }
  free(sounds);

  free(dest);
  exit(0);
}

fileType_t handleCommandLineArgs(int argc, char** argv, char** fileNames, int* numFilesRead, char** outputFileName, char** scalars, int* numScalarsRead) {
  int i;
  char justSawScalar = 0;
  /* starts as CS229, will be converted to wav if we see -w */
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
      if(!justSawScalar) {
        scalars[(*numScalarsRead)++] = argv[i];
        justSawScalar = 1;
      }
      else {
        fileNames[(*numFilesRead)++] = argv[i];
        justSawScalar = 0;
      }
    }
  }
 
  return outputType;
}

void mixSounds(sound_t* dest, sound_t** sounds, float* scalars, int numSounds) {
  int i;
  for(i = 0; i < numSounds; i++) {
    scaleSampleData(sounds[i], scalars[i]);
  }
  deepCopySound(dest, sounds[0]);
  for(i = 1; i < numSounds; i++ ) {
    if(ensureSoundsMixable(dest, sounds[i], dest->fileType) == -1) {
      printSampleRateError();
    }
    convertToFileType(dest->fileType, sounds[i]);
    addSampleData(dest, sounds[i]);
  }
}

char stringsToFloats(char** strings, float* floats, unsigned int numData) {
  char* endPtr;
  while(numData--) {
    floats[numData] = strtof(strings[numData], &endPtr);
    if(floats[numData] == 0 && endPtr == strings[numData]) {
      printScalarConversionError(strings[numData]);
      return 0;
    }
  }
  return 1;
}

void scaleSampleData(sound_t* sound, float scalar) {
  int numSamples = calculateNumSamples(sound);
  if(sound->bitDepth == 8) {
    scaleChars((char*)sound->rawData, numSamples, scalar);
  }
  else if(sound->bitDepth == 16) {
    scaleShorts((short*)sound->rawData, numSamples, scalar);
  }
  else if(sound->bitDepth == 32) {
    scaleLongs((long*)sound->rawData, numSamples, scalar);
  }
}

void scaleChars(char* chars, int numChars, float scalar) {
  int i;
  for(i = 0; i < numChars; i++) {
    chars[i] *= scalar;
  }
}

void scaleShorts(short* shorts, int numShorts, float scalar) {
  int i;
  for(i = 0; i < numShorts; i++) {
    shorts[i] *= scalar;
  }
}

void scaleLongs(long* longs, int numLongs, float scalar) {
  int i;
  for(i = 0; i < numLongs; i++) {
    longs[i] *= scalar;
  }
}

void addSampleData(sound_t* dest, sound_t* addend) {
  int i;
  if(dest->bitDepth != addend->bitDepth) {
    printf("Error: tried to add two sounds of different bit depths\n");
    return;
  }
  if(dest->fileType != addend->fileType) {
    printf("Error: tried to add two sounds of different file types\n");
    return;
  }
  if(dest->bitDepth == 8) {
    for(i = 0; i < calculateTotalDataElements(dest); i++) {
      char* destCharData = (char*)dest->rawData;
      char* addendCharData = (char*)addend->rawData;
      destCharData[i] += addendCharData[i];

    }
  }
  else if(dest->bitDepth == 16) {
    for(i = 0; i < calculateTotalDataElements(dest); i++) {
      short* destShortData = (short*)dest->rawData;
      short* addendShortData = (short*)addend->rawData;
      destShortData[i] += addendShortData[i];
    }
  }
  else if(dest->bitDepth == 32) {
    for(i = 0; i < calculateTotalDataElements(dest); i++) {
      long* destLongData = (long*)dest->rawData;
      long* addendLongData = (long*)addend->rawData;
      destLongData[i] += addendLongData[i];
    }
  }
}

void printTooManyFilesError() {
  fprintf(stderr, "Too many files in input; not enough scalars. (see -h)\n");
}

void printTooManyScalarsError() {
  fprintf(stderr, "Too many scalars in input; not enough files. (see -h)\n");
}

void printScalarConversionError() {
  fprintf(stderr, "Could not convert a given mult to float (see -h for usage)\n"); 
}

void printUsage(char* cmd) {
  printf("Usage: %s [options] mult1 file1 [mult2 file2 ...]", cmd);
} 

void printHelp(char* cmd) {
  printf("Sndmix Help:\n");
  printUsage(cmd);
  printf("\n\n");
 
  printf("Utility:\n");
  printf("This program reads the files passed as arguments, scales the sample data by\n");
  printf("their \"mult\", then adds together their sample data to make a single sound.\n\n");

  printf("If the sounds have unequal channels, zeroed out channels will be added to the\n");
  printf("one with fewer channels.\n");
  printf("If the sounds have different bit depths, the smaller samples are scaled up to\n");
  printf("match the larger ones.\n\n");
  
  printf("Defaults:\n");
  printf("Default output is in CS229 format (see -w option to change). \n");
  printf("Default output is to stdout unless another file is given (see -o)\n");
  printf("If no source files are given, file is read from stdin\n\n");

  printf("Options:\n");
  printf("-h\t\tPrint this screen\n");
  printf("-o [fileName]\tOutput file to fileName\n");
  printf("-w\t\tOutput in WAVE format\n");
}

