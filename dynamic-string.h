// Dynamic string library
// Created by Mário Gažo on 2019-09-25.
//

#ifndef VUT_FIT_IFJ2019_DYNAMIC_STRING_H
#define VUT_FIT_IFJ2019_DYNAMIC_STRING_H

#include <stdbool.h>

typedef struct {
    unsigned int capacity;
    char* text;
} dynamicString_t;

//inicializator
bool dynamicStringInit(dynamicString_t* string);

//uvolnenie
void dynamicStringFree(dynamicString_t* string);

//dynamicke pridanie znaku
bool dynamicStringAddChar(dynamicString_t* string, char c);

//dynamicke pridanie retazca
bool dynamicStringAddString(dynamicString_t* string, const char* source);

#endif //VUT_FIT_IFJ2019_DYNAMIC_STRING_H
