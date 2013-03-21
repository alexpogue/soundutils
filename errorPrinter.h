#ifndef ERROR_PRINTER_H
#define ERROR_PRINTER_H
#include "fileTypes.h"

/**
  Prints error message for sound->error, and nothing if sound->error = NO_ERROR
*/
void printErrorsInSound(sound_t* sound);

/**
  Prints memory allocation error.
*/
void printMemoryError();

/**
  Prints unknown file type error
*/
void printFileTypeError();

/** 
  Prints file does not exist error with the given filepath in the message.
*/
void printFileOpenError(char* filePath);

/**
  Prints error when we cannot load the sound
*/
void printSoundLoadError();

/** 
  Prints error when we encountere an unexpected eof.
*/
void printEofError();

/**
  Prints could not read file error.
*/
void printFileReadError();

/**
  Prints error when user types an unknown option
*/
void printInvalidOptionError(char optionChar);

/**
  Prints error when we encounter an unsupported bit depth
*/
void printBitDepthError();

/**
  Prints error when there is an invalid keyword
*/
void printInvalidKeywordError();

/**
  Prints error when we have no value given for a keyword
*/
void printNoValueError();

/**
  Prints error when the sample data of a cs229 file is misformed
*/
void printSampleDataError();

/**
  Prints error when we try to combine sounds with different sample rates
*/
void printSampleRateError();

#endif
