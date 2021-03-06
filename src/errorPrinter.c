#include "errorPrinter.h"
#include <stdio.h>

void printErrorsInSound(sound_t* sound) {
  if(sound->error == NO_ERROR) {
    return;
  }
  fprintf(stderr, "Error in file: %s\n", sound->fileName);
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
  else if(sound->error == ERROR_ZERO_CHANNELS) {
    printZeroChannelsError();
  }
  else {
    printf("Unforeseen error occurred!\n");
  }
}

void printMemoryError() {
  fprintf(stderr, "Could not allocate memory\n");
}

void printFileTypeError() {
  fprintf(stderr, "Unknown file type\n");
}

void printFileOpenError(char* filePath) {
  fprintf(stderr, "Could not open file: %s\n", filePath);
}

void printSoundLoadError() {
  fprintf(stderr, "Could not load sound\n");
}

void printEofError() {
  fprintf(stderr, "Unexpected end of file\n");
}

void printFileReadError() {
  fprintf(stderr, "Could not read file\n");
}

void printInvalidOptionError(char optionChar) {
  fprintf(stderr, "Unknown option -%c. Use -h for help.\n", optionChar);
}

void printBitDepthError() {
  fprintf(stderr, "Unsupported bit depth encountered\n");
}

void printInvalidKeywordError() {
  fprintf(stderr, "Invalid keyword found in file\n");
}

void printNoValueError() {
  fprintf(stderr, "No value given for a keyword in the file\n");
}

void printSampleDataError() {
  fprintf(stderr, "Corrupted sample data in the file\n");
}

void printSampleRateError() {
  fprintf(stderr, "Incompatible sample rates in sounds\n");
}

void printZeroChannelsError() {
  fprintf(stderr, "Sound is either empty or incorrectly zero channels\n");
}
