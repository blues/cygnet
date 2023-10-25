
#pragma once

#include <float.h>
#include <stdbool.h>
#include <stdint.h>

// Determine our basic floating data type.  In most cases "double" is the right answer, however for
// very small microcontrollers we must use single-precision.
#if defined(FLT_MAX_EXP) && defined(DBL_MAX_EXP)
#if (FLT_MAX_EXP == DBL_MAX_EXP)
#define NOTE_FLOAT
#endif
#elif defined(__FLT_MAX_EXP__) && defined(__DBL_MAX_EXP__)
#if (__FLT_MAX_EXP__ == __DBL_MAX_EXP__)
#define NOTE_FLOAT
#endif
#else
#error What are floating point exponent length symbols for this compiler?
#endif

// Stringify
#define _NOTE_C_STRINGIZE(x) #x
#define NOTE_C_STRINGIZE(x) _NOTE_C_STRINGIZE(x)

// If using a short float, we must be on a VERY small MCU.  In this case, define additional
// symbols that will save quite a bit of memory in the runtime image.
#ifdef NOTE_FLOAT
#define JNUMBER float
#else
#define JNUMBER double
#endif

// Include helpers
#include "n_cjson.h"

// JSON helpers
void JInit(void);
void JCheck(void);
bool JIsPresent(J *rsp, const char *field);
char *JGetString(J *rsp, const char *field);
JNUMBER JGetNumber(J *rsp, const char *field);
J *JGetObject(J *rsp, const char *field);
long int JGetInt(J *rsp, const char *field);
bool JGetBool(J *rsp, const char *field);
JNUMBER JNumberValue(J *item);
char *JStringValue(J *item);
bool JBoolValue(J *item);
long int JIntValue(J *item);
bool JIsNullString(J *rsp, const char *field);
bool JIsExactString(J *rsp, const char *field, const char *teststr);
bool JContainsString(J *rsp, const char *field, const char *substr);
bool JAddBinaryToObject(J *req, const char *fieldName, const void *binaryData, uint32_t binaryDataLen);
bool JGetBinaryFromObject(J *rsp, const char *fieldName, uint8_t **retBinaryData, uint32_t *retBinaryDataLen);
const char *JGetItemName(const J * item);
char *JAllocString(uint8_t *buffer, uint32_t len);
const char *JType(J *item);

#define JTYPE_NOT_PRESENT		0
#define JTYPE_BOOL_TRUE			1
#define JTYPE_BOOL_FALSE		2
#define JTYPE_BOOL              JTYPE_BOOL_TRUE
#define JTYPE_NULL				3
#define JTYPE_NUMBER_ZERO		4
#define JTYPE_NUMBER			5
#define JTYPE_STRING_BLANK		6
#define JTYPE_STRING_ZERO		7
#define JTYPE_STRING_NUMBER		8
#define JTYPE_STRING_BOOL_TRUE 	9
#define JTYPE_STRING_BOOL_FALSE	10
#define JTYPE_STRING			11
#define JTYPE_OBJECT			12
#define JTYPE_ARRAY				13
int JGetType(J *rsp, const char *field);
int JGetItemType(J *item);
int JBaseItemType(int type);
#define JGetObjectItemName(j) (j->string)

// Helper functions for apps that wish to limit their C library dependencies
#define JNTOA_PRECISION (16)
#define JNTOA_MAX       (44)
char * JNtoA(JNUMBER f, char * buf, int precision);
JNUMBER JAtoN(const char *string, char **endPtr);
void JItoA(long int n, char *s);
long int JAtoI(const char *s);
int JB64EncodeLen(int len);
int JB64Encode(char * coded_dst, const char *plain_src,int len_plain_src);
int JB64DecodeLen(const char * coded_src);
int JB64Decode(char * plain_dst, const char *coded_src);

// Hard-wired constants used to specify field types when creating note templates
#define TBOOL           true                 // bool
#define TINT8           11                   // 1-byte signed integer
#define TINT16          12                   // 2-byte signed integer
#define TINT24          13                   // 3-byte signed integer
#define TINT32          14                   // 4-byte signed integer
#define TINT64          18                   // 8-byte signed integer (note-c support depends upon platform)
#define TUINT8          21                   // 1-byte unsigned integer (requires notecard firmware >= build 14444)
#define TUINT16         22                   // 2-byte unsigned integer (requires notecard firmware >= build 14444)
#define TUINT24         23                   // 3-byte unsigned integer (requires notecard firmware >= build 14444)
#define TUINT32         24                   // 4-byte unsigned integer (requires notecard firmware >= build 14444)
#define TFLOAT16        12.1                 // 2-byte IEEE 754 floating point
#define TFLOAT32        14.1                 // 4-byte IEEE 754 floating point (a.k.a. "float")
#define TFLOAT64        18.1                 // 8-byte IEEE 754 floating point (a.k.a. "double")
#define TSTRING(N)      _NOTE_C_STRINGIZE(N) // UTF-8 text of N bytes maximum (fixed-length reserved buffer)
#define TSTRINGV        _NOTE_C_STRINGIZE(0) // variable-length string
bool NoteTemplate(const char *notefileID, J *templateBody);
