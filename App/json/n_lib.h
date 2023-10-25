
#include "json.h"

#pragma once

#define c_null "null"
#define c_null_len 4

#define c_false "false"
#define c_false_len 5

#define c_true "true"
#define c_true_len 4

#define c_nullstring ""
#define c_nullstring_len 0

void *_Malloc(size_t len);
void _Free(void *p);
