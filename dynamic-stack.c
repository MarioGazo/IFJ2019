// dynamic stack implementation
// Created by Mário Gažo on 4.10.19.
//

#include "dynamic-stack.h"
#include <stdlib.h>
#include <stdio.h>

bool stackInit(dynamic_stack_t* s) {
    s->data = calloc(1, sizeof(int));

    if (s->data == NULL) {
        return false;
    } else {
        s->capacity = 1;
        s->top = 0;
        return true;
    }
}

bool stackEmpty(dynamic_stack_t* s) {
    return (s->top == 0);
}

int stackPop(dynamic_stack_t* s) {
    int result = 0;

    if (stackEmpty(s)) {
        return -1;
    } else {
        result = s->data[s->top--];
        s->data = realloc(s->data, --s->capacity * sizeof(int));
        if (s->data == NULL) {
            return -2;
        }
        return result;
    }
}

bool stackPush(dynamic_stack_t* s, int num) {
    s->data = realloc(s->data, ++s->capacity * sizeof(int));

    if (s->data == NULL) {
        return false;
    } else {
        s->data[++s->top] = num;
        return true;
    }
}

void stackFree(dynamic_stack_t* s) {
    free(s->data);
}

void printStack(dynamic_stack_t s) {
    for (int i = 0; i < s.capacity; i++) {
        printf("%d. %d\n",i, s.data[i]);
    }
}

int stackTop(dynamic_stack_t s) {
    return s.data[s.top];
}