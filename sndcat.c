#include <stdio.h>
#include <stdlib.h>
#include "fileUtils.h"

void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType);
void concatenateData(sound_t* s1, sound_t* s2, sound_t* dest);

int main(int argc, char** argv) {
  if(argc < 2) {
    /* read from stdin */
  }
  else {
    int i;
    char** fileNames = NULL;
    int numFiles = 0;
    sound_t** sounds = NULL;
    for(i = 1; i < argc; i++) {
      if(argv[i][0] == '-') {
        if(argv[i][1] == 'h') {
          /* print help and exit(1)*/
        }
        else if(argv[i][1] == 'o') {
          /* set output filename string to argv[i+1] */
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
        char** newFileNames = NULL;
        int newNumFiles = numFiles;
        /* add filename to files to be concatenated */
        ++newNumFiles;
        newFileNames = realloc(fileNames, newNumFiles);
        if(!newFileNames) {
          /* malloc fail */
          exit(1);
        }
        fileNames = newFileNames;
        numFiles = newNumFiles;
        fileNames[numFiles-1] = argv[i];
      }
    }
    sounds = malloc(sizeof(sound_t*) * numFiles);
    for(i = 0; i < numFiles; i++) {
      FILE* fp;
      fp = fopen(fileNames[i], "rb");
      if(!fp) {
        /* file open error */
        exit(1);
      }
      sounds[i] = loadSound(fp, fileNames[i]);
      fclose(fp);
      if(i >= 1) {
        /*concatenateSounds(sounds[i-1], sounds[i], dest, CS229);*/
      }
    }
    sound_t* dest = loadEmptySound();
    concatenateSounds(sounds[0], sounds[1], dest, CS229);
    char* data = (char*)dest->rawData;
    printf("s1 dataSize = %d\n", sounds[0]->dataSize);
    printf("s2 dataSize = %d\n", sounds[1]->dataSize);
    printf("dest dataSize = %d\n", dest->dataSize);
    /*
    for(i = 0; i < calculateNumSamples(dest); i++) {
      printf("%d\n", data[i]);
    }
    */

    for(i = 0; i < numFiles; i++) {
      unloadSound(sounds[i]);
    }
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
     printf("s1 bitdepth = %d\n", s1->bitDepth);
    printf("s2 bitdepth = %d\n", s2->bitDepth);
   dest->bitDepth = s2->bitDepth;
    scaleBitDepth(s2->bitDepth, s1);
    printf("s1 bitdepth = %d\n", s1->bitDepth);
  }
  else {
    dest->bitDepth = s1->bitDepth;
  }
  if(s1->numChannels > s2->numChannels) {
    dest->numChannels = s1->numChannels;
    addZeroedChannels(s1->numChannels - s2->numChannels, s2);
    printf("numChannels = %d\n", s2->numChannels);
  }
  else if(s1->numChannels < s2->numChannels) {
    dest->numChannels = s2->numChannels;
    addZeroedChannels(s2->numChannels - s1->numChannels, s1);
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
