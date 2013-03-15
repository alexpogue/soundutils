#ifndef CS229_UTILS_H
#define CS229_UTILS_H

#include <stdio.h>
#include "fileTypes.h"

/**
  Read fp as a .cs229 file and put it into sound 
  Precondition: file pointer directly after "CS229" header
  Postcondition: file completely read
*/
void cs229Read(FILE* fp, sound_t* sound);

/**
  convert n characters from str to lowercase
*/
void toLowerCase(char* str, size_t n);

/**
  Write sound to file.
  Returns 0 on success.
*/
writeError_t writeCs229File(sound_t* sound, FILE* fp);

#endif
