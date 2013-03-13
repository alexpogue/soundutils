#include <stdio.h>
#include <stdlib.h>
#include "fileUtils.h"

void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType);

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
        /*concatenateSounds(sounds[i-1], sounds[i]);*/
      }
    }
  }
  return 0;
}

/* concatenate sounds and store the concatenated sound into dest. Use the
  format specified by resultType */
void concatenateSounds(sound_t* s1, sound_t* s2, sound_t* dest, fileType_t resultType) {
  /* convert both to resultType */
  if(resultType == CS229) {
    waveToCs229(s1);
    waveToCs229(s2);
  }
  else if(resultType == WAVE) {
    cs229ToWave(s1);
    cs229ToWave(s2);
  }
  if(s1->sampleRate != s2->sampleRate) {
    /* print incompatible sample rate error */
    return;
  }
  if(s1->bitDepth > s2->bitDepth) {    
    scaleBitDepth(s1->bitDepth, s2);
  }
  else if(s1->bitDepth < s2->bitDepth) {
    scaleBitDepth(s2->bitDepth, s1);
  }
  if(s1->numChannels > s2->numChannels) {
    addZeroedChannels(s1->numChannels - s2->numChannels, s2);
  }
  else if(s1->numChannels < s2->numChannels) {
    addZeroedChannels(s2->numChannels - s1->numChannels, s1);
  }
  /* concatenate rawData */
  dest->bitDepth = s1->bitDepth;
  dest->sampleRate = s1->sampleRate;
  dest->fileType = resultType;
  dest->numChannels = s1->numChannels;
  dest->dataSize = s1->dataSize + s2->dataSize;
}

