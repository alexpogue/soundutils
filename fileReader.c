#include "fileReader.h"
#include <stdio.h>
#include <string.h>

readError_t readBytes(void* ptr, size_t n, FILE* file) {
  if(fread(ptr, 1, n, file) < n) {
    /* eof or error in reading */
    if(feof(file)) {
      /*fprintf(stderr, "error: unexpected end of file\n");*/
      return ERROR_EOF;
    }
    else if(ferror(file)) {
      fprintf(stderr, "error reading file\n");
      return ERROR_READING;
    }
  }
  return NO_ERROR;
}

readError_t ignoreLine(FILE* fp) {
  char ignoredChar = 0;
  readError_t error = NO_ERROR;
  while(ignoredChar != '\n' && error == NO_ERROR) {
    error = readBytes(&ignoredChar, 1, fp);
  }
  return error;
}
