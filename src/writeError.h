#ifndef WRITE_ERROR_H
#define WRITE_ERROR_H

typedef enum {
  WRITE_SUCCESS,
  WRITE_ERROR_OPENING,
  WRITE_ERROR_MEMORY,
  WRITE_ERROR_TOO_FEW_CHARS
} writeError_t;

#endif
