#ifndef READ_ERROR_GUARD
#define READ_ERROR_GUARD

/**
  Used as a return value for readBytes(...) and as the error type for file data 
  reading data from a file.
*/
typedef enum {
  /* all clear */
  NO_ERROR,
  /* unexpectedly reached eof */
  ERROR_EOF,
  /* could not read file */
  ERROR_READING,
  /* memory allocation error */
  ERROR_MEMORY,
  /* could not identify filetype */
  ERROR_FILETYPE,
  /* unsupported bit depth */
  ERROR_BIT_DEPTH,
  /* invalid keyword in cs229 file */
  ERROR_INVALID_KEYWORD,
  /* no value given in cs229 file */
  ERROR_NO_VALUE,
  /* error reading sample data in cs229 file */
  ERROR_SAMPLE_DATA
} readError_t;

#endif
