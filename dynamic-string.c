// Dynamic string implementation
// Created by Mário Gažo on 2019-09-25.
//

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

    string->text[string->capacity - 2] = c;
    string->text[string->capacity - 1] = '\0';
    return true;
}

bool dynamicStringAddString(dynamicString_t* string, const char* source) {
    unsigned int sourceLength = strlen(source);
    for (int i = 0; i < sourceLength; i++) {
        if (dynamicStringAddChar(string, source[i])  == false) {
            return false;
        }
    }
    return true;
}
