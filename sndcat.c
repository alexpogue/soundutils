#include <stdio.h>
#include <stdlib.h>
#include "fileUtils.h"

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
      if(i > 0) {
        /*TO BE IMPLEMENTED*/
        /*concatenateSounds(sounds[i-1], sounds[i]); */
      }
    }
  return 0;
}
