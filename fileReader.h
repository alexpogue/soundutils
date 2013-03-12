#ifndef FILE_READER_GUARD
#define FILE_READER_GUARD

#include "readError.h"
#include <stdlib.h>
#include <stdio.h>
/** 
  Read n bytes from file, handles read errors, and put them in ptr. 
  Moves file pointer n positions ahead. 
  return appropriate readError_t
*/
readError_t readBytes(void* ptr, size_t n, FILE* file);

/**
  Ignores the rest of the line and places the file pointer after the newline
*/
readError_t ignoreLine(FILE* fp);

#endif 
