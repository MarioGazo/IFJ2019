// Dynamic string implementation
// Created by Mário Gažo on 2019-09-25.
//

#include "dynamic-string.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool dynamicStringInit(dynamicString_t* string) {
    string->text = malloc(1);

    if (string->text == NULL) {
        return false;
    } else {
        string->capacity = 1;
        string->textSize = 0;
        string->text[string->textSize] = '\0';
        return true;
    }
}

void dynamicStringFree(dynamicString_t* string) {
    free(string->text);
}

bool dynamicStringAddChar(dynamicString_t* string, char c) {
    string->text = realloc(string->text,++string->capacity);

    if (string->text == NULL) {
        return false;
    }

    string->text[string->textSize++] = c;
    string->text[string->textSize] = '\0';
    return true;
}

bool dynamicStringAddString(dynamicString_t* string, const char* source) {

    unsigned int sourceLength = strlen(source);
    for (int i = 0; i < sourceLength; i++) {
        dynamicStringAddChar(string, source[i]);
    }
    return true;
}
