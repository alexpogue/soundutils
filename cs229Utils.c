#include "cs229Utils.h"
#include "fileReader.h"
#include "readError.h"
#include "writeError.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

typedef enum {
  KEYWORD_SAMPLES,
  KEYWORD_CHANNELS,
  KEYWORD_BITRES,
  KEYWORD_SAMPLERATE,
  KEYWORD_STARTDATA,
  KEYWORD_COMMENT,
  KEYWORD_ERROR,
  KEYWORD_BADVALUE
} keyword_t;

typedef enum {
  CS229_NO_ERROR,
  CS229_DONE_READING,
  CS229_ERROR_EOF,
  CS229_ERROR_INVALID_VALUE,
  CS229_ERROR_READING
} cs229ReadStatus_t;

typedef struct {
  void* data;
  unsigned short numSamples;
  unsigned short sampleRate;
  unsigned char numChannels;
  unsigned char bitres;
} cs229Data_t;
   

/**
  Reads the samples from the startdata section into the given cs229Data_t. 
  Appropriate cs229ReadStatus_t is returned.
  Precondition: file pointer directly before a sample
  Postcondition: file pointer directly after a sample
*/
cs229ReadStatus_t readSamples(cs229Data_t* cd, FILE* fp); 

/**
  Reads "keyword[whitespace]value" and places null-terminated keyword and value 
  into the char* parameters "keyword" and "value". Will only read keywordLen-1 
  chars into keyword, and valueLen-1 chars into value. If we can't find a value 
  or keyword, or if they are longer than the given "Len"gth, first character is 
  a null terminator. Returns readError_t indicating any errors that occurred.
  Precondition: file pointer is at the beginning of line
  Postcondition: file pointer is at beginning of next line
*/
readError_t getKeywordValue(char* keyword, size_t keywordLen, char* value, size_t valueLen, FILE* fp);

/** 
  Reads characters from file into str until we get to a whitespace character. One 
  whitespace character is read from file and included in str after returning.
*/
readError_t readUntilWhitespace(char* str, size_t n, FILE* file);

/**
  Read until we encounter a character that is not a tab or space. New lines are
  NOT whitespace and will cause this method to return.
*/
readError_t readUntilNonWhitespace(char* nonWhite, FILE* fp);

/**
  Stores the keyword/value pair of char* in the given cs229Data_t. Returns the 
  stored keyword_t on success. If the given keyword char* has no corresponding
  keyword_t, return KEYWORD_UNKNOWN. If the value cannot be evaluated as the 
  proper integral type, KEYWORD_ERROR is returned. 
*/
keyword_t storeKeywordValue(char* keyword, char* value, cs229Data_t* cd);

/**
  Returns the keyword_t that matches the given str.
*/
keyword_t strToKeyword(char* str);

/**
  Gets the keyword and value from fp and puts it into the given cs229Data_t, 
  then returns the keyword that we filled in.
*/
keyword_t readKeywordValue(cs229Data_t* cd, FILE* fp);

/**
  Translates cd over to sound
*/
void cs229ToSound(cs229Data_t* cd, sound_t* sound);

/**
  Calculates the data size from numsamples, numchannels, and bitdepth
*/
unsigned int calculateDataSize(cs229Data_t* cd);

/**
  Convert the first num characters in p to lowercase.
*/
void toLowerCase(char* p, size_t num) {
  while(num) if(p[--num]>='A' && p[num]<='Z') p[num]+='a'-'A';
}

void cs229Read(FILE* fp, sound_t* sound) {
  cs229Data_t* cData = (cs229Data_t*)malloc(sizeof(cs229Data_t));
  keyword_t keyword;
  cs229ReadStatus_t sampleReadStatus;

  /*ignore newline after "CS229" header */
  sound->error = ignoreLine(fp);
  if(sound->error != NO_ERROR) return;

  keyword = readKeywordValue(cData, fp);
  while(keyword != KEYWORD_STARTDATA) {
    if(keyword == KEYWORD_ERROR) {
      sound->error = ERROR_INVALID_KEYWORD;
      return;
    }
    if(keyword == KEYWORD_BADVALUE) {
      sound->error = ERROR_NO_VALUE;
      return;
    }
    keyword = readKeywordValue(cData, fp);
  }
  if(cData->bitres != 8 && cData->bitres != 16 && cData->bitres != 32) {
    sound->error = ERROR_BIT_DEPTH;
    return;
  }
  cData->data = malloc(calculateDataSize(cData));
  sampleReadStatus = readSamples(cData, fp);
  if(sampleReadStatus != CS229_DONE_READING && sampleReadStatus != CS229_NO_ERROR) {
    sound->error = ERROR_SAMPLE_DATA;
    return;
  }
  
  cs229ToSound(cData, sound);
  free(cData);
}

void cs229ToSound(cs229Data_t* cd, sound_t* sound) {
  sound->sampleRate = cd->sampleRate;
  sound->numChannels = cd->numChannels;
  sound->bitDepth = cd->bitres;
  sound->dataSize = calculateDataSize(cd);
  sound->rawData = cd->data;
}

unsigned int calculateDataSize(cs229Data_t* cd) {
  if(cd->numChannels == 0 || cd->bitres == 0) return 0;
  return (unsigned int)cd->numSamples * (unsigned int)cd->numChannels * (unsigned int)cd->bitres / 8;
}

int longToShort(long makeMeAShort) {
  if(makeMeAShort < SHRT_MIN
    || makeMeAShort > SHRT_MAX) {
    printf("%ld is out of int range\n", makeMeAShort); 
    return !makeMeAShort;
  }
  return (int)makeMeAShort;
}

signed char longToChar(long makeMeAChar) {
  if(makeMeAChar < SCHAR_MIN
    || makeMeAChar > SCHAR_MAX) {
    printf("%ld is out of char range\n", makeMeAChar);
    return !makeMeAChar;
  }
  return (char)makeMeAChar;
}

unsigned int longToUShort(unsigned long makeMeAUShort) {
  if(makeMeAUShort > USHRT_MAX) {
    printf("%ld is out of uint range", makeMeAUShort);
    return !makeMeAUShort;
  }
  return (unsigned int)makeMeAUShort;
}

unsigned char longToUChar(unsigned long makeMeAUChar) {
  if(makeMeAUChar > UCHAR_MAX) {
    printf("%ld is out of uchar range", makeMeAUChar);
    return !makeMeAUChar;
  }
  return (unsigned char)makeMeAUChar;
}


keyword_t storeKeywordValue(char* keywordStr, char* valueStr, cs229Data_t* cd) {
  keyword_t kw = strToKeyword(keywordStr);
  char* afterNumber;
  unsigned long val;
  unsigned int samples, sampleRate;
  unsigned char channels, bitres;
  if(kw == KEYWORD_STARTDATA || kw == KEYWORD_COMMENT) {
    /* don't try to read value for startdata or comment, there isn't one! */
    return kw;
  }
  val = strtol(valueStr, &afterNumber, 10);
  if(valueStr[0] == '\0' || afterNumber[0] != '\0') {
    /* not a valid number */
    return KEYWORD_BADVALUE;
  }
  switch(kw) {
    case KEYWORD_SAMPLES:
      samples = longToUShort(val);
      if(val != samples) return KEYWORD_BADVALUE;
      cd->numSamples = samples;
      break;
    case KEYWORD_CHANNELS:
      channels = longToUChar(val);
      if(val != channels) return KEYWORD_BADVALUE;
      cd->numChannels = channels;
      break;
    case KEYWORD_BITRES:
      bitres = longToUShort(val);
      if(bitres != val) return KEYWORD_BADVALUE;
      cd->bitres = bitres;
      break;
    case KEYWORD_SAMPLERATE:
      sampleRate = longToUShort(val);
      if(val != sampleRate) return KEYWORD_BADVALUE;
      cd->sampleRate = sampleRate;
      break;
    default:
      return KEYWORD_ERROR;
  }
  return kw;
}

keyword_t readKeywordValue(cs229Data_t* cd, FILE* fp) {
  /* keyword is 12 bytes to hold largest keyword: "samplerate"(10), plus an 
  invalid character to ensure valid keyword, and finaly a null 
  terminator */
  char keywordStr[12];
  /* value is 13 bytes to hold "-2147483647"(11), plus invalid character to 
  ensure valid value, plus a null terminator */
  char valueStr[13];
  keyword_t kw;
  readError_t error = getKeywordValue(keywordStr, 12, valueStr, 13, fp);
  if(error != NO_ERROR) {
    return KEYWORD_ERROR;
  }
  toLowerCase(keywordStr, strlen(keywordStr));
  kw = storeKeywordValue(keywordStr, valueStr, cd);
  return kw;
}

keyword_t strToKeyword(char* str) {
  if(strcmp(str, "samples") == 0) {
    return KEYWORD_SAMPLES;
  }
  else if(strcmp(str, "channels") == 0) {
    return KEYWORD_CHANNELS;
  }
  else if(strcmp(str, "bitres") == 0) {
    return KEYWORD_BITRES;
  }
  else if(strcmp(str, "samplerate") == 0) {
    return KEYWORD_SAMPLERATE;
  }
  else if(strcmp(str, "startdata") == 0) {
    return KEYWORD_STARTDATA;
  }
  else if(strcmp(str, "#") == 0) {
    return KEYWORD_COMMENT;
  }
  else {
    return KEYWORD_ERROR;
  }
}

readError_t getKeywordValue(char* keyword, size_t keywordLen, char* value, size_t valueLen, FILE* fp) {
  /* 
  1. copy characters until first whitespace
  2. store copied chars into keyword
  3. keep scanning for next non-whitespace
  4. copy non-whitespace until another whitespace or newline
  5. store copied chars in value
  */
  size_t length;
  char finalChar, firstValueChar;
  readError_t error = readUntilWhitespace(keyword, keywordLen, fp);
  if(error != NO_ERROR) {
    return error;
  }
  length = strlen(keyword);
  finalChar = keyword[length-1];
  if(finalChar != ' ' && finalChar != '\t' && finalChar != '\n') {
    printf("We don't have enough room!\n");
    /* here, we didn't have enough room in keyword */
    keyword[0] = 0;
    return NO_ERROR;
  }
  /* get rid of whitespace on the end of keyword */
  keyword[length-1] = 0;
  if(length > 2 && keyword[length-2] == '\r') {
    keyword[length-2] = 0;
  }
  if(finalChar == '\n' && keyword[0] != '\r' && length > 1) {
    value[0] = 0; /* null first value char to show value was not found */
    return NO_ERROR;
  }
  else if(finalChar == '\n' && (length == 1 || keyword[0] == '\r')) {
    /* blank line, regard as comment */
    keyword[0] = '#';
    return NO_ERROR;
  }

  error = readUntilNonWhitespace(&firstValueChar, fp);
  if(error != NO_ERROR) {
    return error;
  }
  if(firstValueChar == '\n') {
    /* did not find a value before we find newline */
    value[0] = 0;
    return NO_ERROR;
  }
  value[0] = firstValueChar;
  /* already read first character of value */
  error = readUntilWhitespace(&value[1], valueLen - 1, fp);
  if(error != NO_ERROR) {
    return error;
  }
  length = strlen(value);
  finalChar = value[length-1];
  if(finalChar != ' ' && finalChar != '\t' && finalChar != '\n') {
    /* ensure we read until a whitespace character */
    value[0] = 0;
    return NO_ERROR;
  }
  if(finalChar != '\n') {
    error = ignoreLine(fp);
    if(error != NO_ERROR) {
      return error;
    }
  }
  value[length-1] = 0;
  if(length > 2 && value[length-2] == '\r') {
    value[length-2] = 0;
  }
  return NO_ERROR;
}

readError_t readUntilNonWhitespace(char* nonWhite, FILE* fp) {
  do {
    readError_t error = readBytes(nonWhite, 1, fp);
    if(error != NO_ERROR) {
      return error;
    }
  } while(*nonWhite == ' ' || *nonWhite == '\t');
  return NO_ERROR;
}

/**
  Reads n-1 characters or until and including space, tab, or newline, then pad 
  with a null terminator. When a read error occurs, returns readError_t early 
  and does not null terminate the string.
*/
readError_t readUntilWhitespace(char* str, size_t n, FILE* file) {
  char byte;
  int i = 0;
  readError_t readError = NO_ERROR;
  do {
    readError = readBytes(&byte, 1, file);
    if(readError != NO_ERROR) {
      return readError;
    }
    str[i++] = byte;
  } while(byte != '\t' && byte != ' ' && byte != '\n' && i < n - 1);
  str[i] = 0;
  return NO_ERROR;
}

/* TODO: go through this method with paper and pencil and clean it */
cs229ReadStatus_t readSample(cs229Data_t* cd, int index, FILE* fp) {
  int i;
  readError_t error;
  /* to hold "-2147483647"(11), + additional char to ensure validity, + '\0' */
  char dataStr[13], length, valChar, *afterNumber; 
  unsigned char lastChar;
  short valShort;
  long valLong;

  char* dataChars;
  short* dataShorts;
  long* dataLongs;

  for(i = 0; i < (cd->numChannels); i++) {
    /* to reach the next sample data */
    error = readUntilNonWhitespace(&dataStr[0], fp);
    error = readUntilWhitespace(&dataStr[1], 12, fp);
    /* it might be okay to reach eof after the final channel is read */
    if(error == ERROR_EOF) {
      return CS229_DONE_READING;
    }
    if(error == ERROR_READING) {
      return CS229_ERROR_READING;
    }
    length = strlen(dataStr);
    lastChar = dataStr[length-1];
    if(lastChar != ' ' && lastChar != '\t' && lastChar != '\n') {
      return CS229_ERROR_INVALID_VALUE;
    }
    dataStr[length-1] = 0;

    if(length > 2 && dataStr[length-2] == '\r') { 
      dataStr[length-2] = 0;
    }

    valLong = strtol(dataStr, &afterNumber, 10);
    if(dataStr[0] == '\0' || afterNumber[0] != '\0') {
      /* not a valid number */
      return CS229_ERROR_INVALID_VALUE;
    }
    switch(cd->bitres) {
      case 8:
        dataChars = (char*)cd->data;
        valChar = longToChar(valLong);
        if(valChar != valLong) return CS229_ERROR_INVALID_VALUE;
        dataChars[index + i] = valChar;
        break;
      case 16:
        dataShorts = (short*)cd->data;
        valShort = longToShort(valLong);
        if(valShort != longToShort(valLong)) return CS229_ERROR_INVALID_VALUE;
        dataShorts[index + i] = valShort;
        break;
      case 32:
        dataLongs = (long*)cd->data;
        dataLongs[index + i] = valLong;
        break;
      default:
        /* we should have caught invalid bitres before calling this function */
        return CS229_ERROR_INVALID_VALUE;
    }
  }
  if(lastChar != '\n') {
    error = ignoreLine(fp);
  }
  if(error == ERROR_EOF) {
    /* eof error is OK, we just handled the final sample*/
    return CS229_DONE_READING;
  }
  if(error != NO_ERROR) {
    return CS229_ERROR_READING;
  }
  return CS229_NO_ERROR;
}

cs229ReadStatus_t readSamples(cs229Data_t* cd, FILE* fp) {
  int i;
  cs229ReadStatus_t status = CS229_NO_ERROR;
  for(i = 0; status == CS229_NO_ERROR; i++) {
    status = readSample(cd, i * cd->numChannels, fp);
  }
  cd->numSamples = i-1;
  return status;
}
