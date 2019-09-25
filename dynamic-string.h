// Dynamic string library
// Created by Mário Gažo on 2019-09-25.
//

#ifndef VUT_FIT_IFJ2019_DYNAMIC_STRING_H
#define VUT_FIT_IFJ2019_DYNAMIC_STRING_H

#include <stdbool.h>

typedef struct dynamicString {
    unsigned int textSize;
    unsigned int capacity;
    char* text;
} dynamicString_t;

bool dynamicStringInit(dynamicString_t* string);

void dynamicStringFree(dynamicString_t* string);

bool dynamicStringAddChar(dynamicString_t* string, char c);

bool dynamicStringAddString(dynamicString_t* string, const char* source);

#endif //VUT_FIT_IFJ2019_DYNAMIC_STRING_H
