#include <stdio.h>
#include <stdlib.h>
#include "fileUtils.h"
#include "errorPrinter.h"

void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType);
void concatenateData(sound_t* s1, sound_t* s2, sound_t* dest);

int main(int argc, char** argv) {
  if(argc < 2) {
    /* read from stdin */
  }
  else {
    int i;
    char** fileNames = NULL;
    char* outputFileName = NULL;
    int fileLimit = 2;
    int numFiles = 0;
    sound_t* dest;
    sound_t** sounds = NULL;
    fileNames = (char**)malloc(fileLimit * sizeof(char*));
    if(!fileNames) {
      printf("Malloc failed!\n");
      exit(1);
    }
    for(i = 1; i < argc; i++) {
      if(argv[i][0] == '-') {
        if(argv[i][1] == 'h') {
          /* print help and exit(1)*/
        }
        else if(argv[i][1] == 'o') {
          outputFileName = argv[i+1];
          /* don't reread the file name as an input file */
          ++i;
        }
        else if(argv[i][1] == 'w') {
          /* set wav output flag */
        }
        else {
          /* unknown command line flag error */
        }
      }
      else {
        int newNumFiles = numFiles + 1;
        if(newNumFiles > fileLimit) {
          fileLimit *= 2;
          char** newFileNames = NULL;
          newFileNames = (char**)realloc(fileNames, fileLimit * sizeof(char*));
          if(!newFileNames) {
            printf("Malloc failed!\n");
            free(fileNames);
            exit(1);
          }
          fileNames = newFileNames;
        }
        numFiles = newNumFiles;
        fileNames[numFiles-1] = argv[i];
      }
    }
    sounds = malloc(sizeof(sound_t*) * numFiles);
    if(!sounds) {
      printf("Malloc failed!\n");
      free(fileNames);
      exit(1);
    }
    for(i = 0; i < numFiles; i++) {
      FILE* fp;
      fp = fopen(fileNames[i], "rb");
      if(!fp) {
        fprintf(stderr, "File %s could not be opened\n", fileNames[i]);
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
    if(sounds[0]->error == NO_ERROR && sounds[1]->error == NO_ERROR) {
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
      printErrorsInSound(sounds[0]);
      printErrorsInSound(sounds[1]);
    }
    for(i = 0; i < numFiles; i++) {
      unloadSound(sounds[i]);
    }
    free(sounds);
  }
  return 0;
}

/* concatenate sounds and store the concatenated sound into dest. Use the
  format specified by resultType */
void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType) {
  if(resultType == CS229) {
    waveToCs229(s1);
    waveToCs229(s2);
  }
  else if(resultType == WAVE) {
    cs229ToWave(s1);
    cs229ToWave(s2);
  }
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
    fprintf(stderr, "Malloc failed\n");
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
