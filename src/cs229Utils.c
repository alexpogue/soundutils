#include "cs229Utils.h"
#include "fileReader.h"
#include "readError.h"
#include "writeError.h"
#include "fileUtils.h"
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
  CS229_ERROR_INVALID_DATA,
  CS229_ERROR_READING,
  CS229_ERROR_NOT_ENOUGH_DATA,
  CS229_ERROR_TOO_MUCH_DATA
} cs229ReadStatus_t;

typedef struct {
  void* data;
  unsigned long numSamples;
  unsigned short sampleRate;
  unsigned char numChannels;
  unsigned char bitres;
} cs229Data_t;
   

/**
  Reads the samples from the startdata up to sampleLimit samples. Reads samples
  into the given cs229Data_t. Appropriate cs229ReadStatus_t is returned and 
  samplesFilled is modified to tell how many samples we read.
  Precondition: file pointer directly before a sample
  Postcondition: file pointer directly after a sample
*/
cs229ReadStatus_t readSamples(cs229Data_t* cd, int sampleLimit, int* samplesFilled, FILE* fp); 

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
void cs229ToSound(cs229Data_t* cd, sound_t* sound, cs229ReadStatus_t status);

/** 
  Convert status to its best-fit readError_t. 
*/
readError_t cs229ReadStatusToReadError(cs229ReadStatus_t status);

/**
  Calculates the data size from numsamples, numchannels, and bitdepth
*/
unsigned int calculateDataSize(cs229Data_t* cd);

/**
  Takes a 2d array of singleSamples and concatenates them together into result.
  Result will be the sampleData as defined by the CS229 spec under "StartData".
*/
int makeSampleString(char* result, char** singleSamples, sound_t* sound);

/**
  Convert the first num characters in p to lowercase.
*/
void toLowerCase(char* p, size_t num) {
  while(num) if(p[--num]>='A' && p[num]<='Z') p[num]+='a'-'A';
}

void cs229Read(FILE* fp, sound_t* sound) {
  cs229Data_t* cData = malloc(sizeof(cs229Data_t));
  keyword_t keyword;
  cs229ReadStatus_t sampleReadStatus = CS229_NO_ERROR;
  long bytesAvailable = 16;
  long bytesUsed = 0;
  int samplesRead = 0;
  void* newData = NULL;

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

  cData->data = NULL;
  do {
    int bytesPerSample = cData->numChannels * cData->bitres / 8;
    int sampleLimit;
    bytesAvailable *= 2;
    if(bytesPerSample == 0) {
      /* to prevent divide by zero error */
      sound->error = ERROR_ZERO_CHANNELS;
      break;
    }
    sampleLimit = bytesAvailable / bytesPerSample;
    newData = realloc(cData->data, bytesAvailable);
    if(!newData) {
      sound->error = ERROR_MEMORY;
      free(cData);
      return;
    }
    cData->data = newData;
    sampleReadStatus = readSamples(cData, sampleLimit, &samplesRead, fp);
  } while(sampleReadStatus == CS229_NO_ERROR && sound->error == NO_ERROR);
  cData->numSamples = samplesRead;
  bytesUsed = samplesRead * cData->numChannels * cData->bitres / 8;
  /* reallocate to fix overestimation of data size from do/while loop */
  newData = realloc(cData->data, bytesUsed);
  if(!newData && bytesUsed > 0) {
    sound->error = ERROR_MEMORY;
    free(cData);
    return;
  }
  cData->data = newData;
      
  cs229ToSound(cData, sound, sampleReadStatus);
  free(cData);
}

void cs229ToSound(cs229Data_t* cd, sound_t* sound, cs229ReadStatus_t status) {
  sound->sampleRate = cd->sampleRate;
  sound->numChannels = cd->numChannels;
  sound->bitDepth = cd->bitres;
  sound->dataSize = calculateDataSize(cd);
  sound->rawData = cd->data;
  if(sound->error == NO_ERROR) {
    sound->error = cs229ReadStatusToReadError(status);
  }
}

readError_t cs229ReadStatusToReadError(cs229ReadStatus_t status) {
  if(status == CS229_NO_ERROR || status == CS229_DONE_READING) {
    return NO_ERROR;
  }
  else if(status == CS229_ERROR_EOF) {
    return ERROR_EOF; 
  }
  else if(status == CS229_ERROR_INVALID_DATA 
    || status == CS229_ERROR_NOT_ENOUGH_DATA
    || status == CS229_ERROR_TOO_MUCH_DATA) {
    return ERROR_SAMPLE_DATA;
  }
  else if(status == CS229_ERROR_READING) {
    return ERROR_READING;
  }
  else {
    fprintf(stderr, "Unhandled cs229ReadStatus_t enum value\n");
    return ERROR_READING;
  }
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
    printf("%ld is out of ushort range\n", makeMeAUShort);
    return !makeMeAUShort;
  }
  return (unsigned int)makeMeAUShort;
}

unsigned char longToUChar(unsigned long makeMeAUChar) {
  if(makeMeAUChar > UCHAR_MAX) {
    printf("%ld is out of uchar range\n", makeMeAUChar);
    return !makeMeAUChar;
  }
  return (unsigned char)makeMeAUChar;
}

keyword_t storeKeywordValue(char* keywordStr, char* valueStr, cs229Data_t* cd) {
  keyword_t kw;
  char* afterNumber;
  unsigned long val;
  unsigned int samples, sampleRate;
  unsigned char channels, bitres;

  kw = strToKeyword(keywordStr);
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
      samples = val;
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
  else if(str[0] == '#') {
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
  else if( finalChar == '\n' && (length == 1 || keyword[0] == '\r') ) {
    /* blank line, regard as comment */
    keyword[0] = '#';
    return NO_ERROR;
  }
  else if( ((finalChar == ' ' || finalChar == '\t') && length == 1) || keyword[0] == '#') {
    /* line started with whitespace or began with a #, also regard as comment */
    keyword[0] = '#';
    error = ignoreLine(fp);
    if(error != NO_ERROR) {
      return error;
    }
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
  with a null terminator. When a read error occurs, adds null terminator and 
  returns readError_t early.
*/
readError_t readUntilWhitespace(char* str, size_t n, FILE* file) {
  char byte;
  int i = 0;
  readError_t readError = NO_ERROR;
  do {
    readError = readBytes(&byte, 1, file);
    if(readError != NO_ERROR) {
      str[i] = 0;
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

  lastChar = 0;

  for(i = 0; i < (cd->numChannels); i++) {
    /* to reach the next sample data */
    error = readUntilNonWhitespace(&dataStr[0], fp);
    error = readUntilWhitespace(&dataStr[1], 12, fp);
    if(error == ERROR_EOF) {
      return CS229_DONE_READING;
    }
    if(error == ERROR_READING) {
      return CS229_ERROR_READING;
    }
    length = strlen(dataStr);
    lastChar = dataStr[length-1];
    if(lastChar != ' ' && lastChar != '\t' && lastChar != '\n') {
      return CS229_ERROR_INVALID_DATA;
    }
    if(lastChar == '\n' && i != cd->numChannels - 1) {
      /* premature newline */
      return CS229_ERROR_NOT_ENOUGH_DATA;
    }
    else if(lastChar != '\n' && lastChar != '\t' && lastChar != ' '  && i == cd->numChannels - 1) {
      /* late newline */
      return CS229_ERROR_TOO_MUCH_DATA;
    }
    dataStr[length-1] = 0;
    if(length > 2 && dataStr[length-2] == '\r') { 
      dataStr[length-2] = 0;
    }

    valLong = strtol(dataStr, &afterNumber, 10);
    if(dataStr[0] == '\0' || afterNumber[0] != '\0') {
      /* not a valid number */
      return CS229_ERROR_INVALID_DATA;
    }
    switch(cd->bitres) {
      case 8:
        dataChars = (char*)cd->data;
        valChar = longToChar(valLong);
        if(valChar != valLong) return CS229_ERROR_INVALID_DATA;
        dataChars[index + i] = valChar;
        break;
      case 16:
        dataShorts = (short*)cd->data;
        valShort = longToShort(valLong);
        if(valShort != longToShort(valLong)) return CS229_ERROR_INVALID_DATA;
        dataShorts[index + i] = valShort;
        break;
      case 32:
        dataLongs = (long*)cd->data;
        dataLongs[index + i] = valLong;
        break;
      default:
        /* we should have caught invalid bitres before calling this function */
        return CS229_ERROR_INVALID_DATA;
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

/* TODO: give cs229Data a status member and modify it's status instead of returning */
cs229ReadStatus_t readSamples(cs229Data_t* cd, int sampleLimit, int* samplesFilled, FILE* fp) {
  int i;
  int samplesFilledThisTime = 0;
  cs229ReadStatus_t status = CS229_NO_ERROR;
  for(i = *samplesFilled; status == CS229_NO_ERROR && i < sampleLimit; i++) {
    status = readSample(cd, i * cd->numChannels, fp);
    ++samplesFilledThisTime;
  }
  if(status != CS229_NO_ERROR) {
    /* we did not fill the last sample if we had an error */
    samplesFilledThisTime -= 1;
  }
  *samplesFilled += (samplesFilledThisTime);
  return status;
}

int getMaxCharsIn8Bit() {
  /* maxChars is "-127" */
  return 4;
}
int getMaxCharsIn16Bit() {
  /* maxChars is "-32768" */
  return 6;
}
int getMaxCharsIn32Bit() {
  /* maxChars is "-2147483647" */
  return 11;
}

int getMaxCharsPerSample(sound_t* sound) {
  int maxCharsInEachNum, charsForAllNums, charsForSpaces, charsForNewline;
  maxCharsInEachNum = 0;
  if(sound->bitDepth == 8) {
    maxCharsInEachNum = getMaxCharsIn8Bit();
  }
  else if(sound->bitDepth == 16) {
    maxCharsInEachNum = getMaxCharsIn16Bit();
  }
  else if(sound->bitDepth == 32) {
    maxCharsInEachNum = getMaxCharsIn32Bit();
  }
  charsForAllNums = maxCharsInEachNum * sound->numChannels;
  charsForSpaces = sound->numChannels;
  charsForNewline = 1;
  return charsForAllNums + charsForSpaces + charsForNewline;
}

/* TODO: improve bounds checking */
int getSamplesInCs229Format(sound_t* sound, char* str, int size) {
  int numSamples = calculateNumSamples(sound);
  int i, j;
  int charCount = 0;
  char** sampleStrings = malloc(sizeof(char**) * numSamples * sound->numChannels);
  for(i = 0; i < numSamples; i++) {
    int currSampleIndex = i * sound->numChannels;
    int maxCharsPerData;
    for(j = 0; j < sound->numChannels; j++) {
      int currDataIndex = currSampleIndex + j;
      if(sound->bitDepth == 8) {
        char* charData = (char*)sound->rawData;
        /* 6 chars to fit "-127 \0" */
        maxCharsPerData = 6;
        sampleStrings[currDataIndex] = malloc(maxCharsPerData);
        snprintf(sampleStrings[currDataIndex], maxCharsPerData, "%d ", charData[currDataIndex]);
      }
      else if(sound->bitDepth == 16) {
        short* shortData = (short*)sound->rawData;
        /* 8 chars to fit "-32767 \0" */
        maxCharsPerData = 8;
        sampleStrings[currDataIndex] = malloc(maxCharsPerData);
        snprintf(sampleStrings[currDataIndex], maxCharsPerData, "%hd ", shortData[currDataIndex]);
      }
      else if(sound->bitDepth == 32) {
        long* longData = (long*)sound->rawData;
        /* 13 chars to fit "-2147483648 \0" */
        maxCharsPerData = 13;
        sampleStrings[currDataIndex] = malloc(maxCharsPerData);
        snprintf(sampleStrings[currDataIndex], maxCharsPerData, "%ld ", longData[currDataIndex]);
      }
      if(charCount >= size) {
        printf("Overflow while writing samples, code red.\n");
        printf("tried to read %d chars\n", charCount);
        return charCount;
      }
    }
  }
  charCount = makeSampleString(str, sampleStrings, sound); 
  for(i = 0; i < numSamples * sound->numChannels; i++) {
    free(sampleStrings[i]);
  }
  free(sampleStrings);
  return charCount;
}

int makeSampleString(char* result, char** singleSamples, sound_t* sound){
  int i, j, k, pos, numSamples;
  numSamples = calculateNumSamples(sound);
  pos = 0;
  for(i = 0; i < numSamples; i++) {
    int sampleIndex = i * sound->numChannels;
    for(j = 0; j < sound->numChannels; j++) {
      int dataIndex = sampleIndex + j;
      int currLen = strlen(singleSamples[dataIndex]);
      for(k = 0; k < currLen; k++) {
        result[pos++] = singleSamples[dataIndex][k];
      }
    }
    result[pos++] = '\n';
  }
  result[pos++] = 0;
  return pos;
}

int getMaxSizeSamples(sound_t* sound) {
  int nullTerminatorByte = 1;
  return getMaxCharsPerSample(sound) * calculateNumSamples(sound) + nullTerminatorByte;
}
      
writeError_t writeCs229File(sound_t* sound, FILE* fp) {
  int maxSizeSamples = getMaxSizeSamples(sound);
  char* sampleData = NULL;
  sampleData = malloc(maxSizeSamples);
  if(!sampleData) {
    return WRITE_ERROR_MEMORY;
  } 
  sampleData[0] = 0;
  getSamplesInCs229Format(sound, sampleData, maxSizeSamples);
  fprintf(fp, "CS229\n");
  fprintf(fp, "Samples %d\n", calculateNumSamples(sound));
  fprintf(fp, "Channels %d\n", sound->numChannels);
  fprintf(fp, "BitRes %d\n", sound->bitDepth);
  fprintf(fp, "SampleRate %ld\n", sound->sampleRate);
  fprintf(fp, "StartData\n");
  fprintf(fp, "%s", sampleData);
  free(sampleData);
  return WRITE_SUCCESS;
}
