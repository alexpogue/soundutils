#include "errorPrinter.h"
#include <stdio.h>

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
