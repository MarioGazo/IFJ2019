/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-string.c
 * @author Mario Gazo (xgazom00)
 * @brief Dynamic string implementation
 */

#include "dynamic-string.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool dynamicStringInit(dynamicString_t* string) {
    string->text = calloc(1, sizeof(char));

    if (string->text == NULL) {
        return false;
    } else {
        string->capacity = 1;
        string->text[0] = '\0';
        return true;
    }
}

void dynamicStringFree(dynamicString_t* string) {
    free(string->text);
}

bool dynamicStringAddChar(dynamicString_t* string, int c) {
    string->text = realloc(string->text,++string->capacity);

    if (string->text == NULL) {
        return false;
    }

    string->text[string->capacity - 2] = c;
    string->text[string->capacity - 1] = '\0';
    return true;
}

bool dynamicStringAddString(dynamicString_t* string, const char* source) {
    unsigned int sourceLength = strlen(source);
    for (unsigned int i = 0; i < sourceLength; i++) {
        if (dynamicStringAddChar(string, source[i])  == false) {
            return false;
        }
    }
    return true;
}

char* dynamicStringGetText(dynamicString_t string) {
    return string.text;
}

int dynamicStringStrCmp(dynamicString_t string1, dynamicString_t string2) {
    return strcmp(dynamicStringGetText(string1),dynamicStringGetText(string2));
}
