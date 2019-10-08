// lexical analysis definition
// Created by Mário Gažo on 2019-10-03.
//

#ifndef VUT_FIT_IFJ2019_SCANNER_H
#define VUT_FIT_IFJ2019_SCANNER_H

#include <stdbool.h>
#include <stdio.h>
#include "dynamic-string.h"
#include "dynamic-stack.h"

// returning next token
typedef union {
    int intValue;
    double doubleValue;
    dynamicString_t word;
} tokenAttribute_t;

typedef enum parserState {
    Start,                 //  0 S
    Plus,                  //  1 +
    Minus,                 //  2 -
    Multiply,              //  3 *
    Colon,                 //  4 :
    LeftBracket,           //  5 (
    RightBracket,          //  6 )
    Comma,                 //  7 ,
    DivideWRest,           //  8 /
    DivideWORest,          //  9 //
    Exclamation,           // 10 !
    NotEqual,              // 11 !=
    Smaller,               // 12 <
    SmallerOrEqual,        // 13 <=
    Bigger,                // 14 >
    BiggerOrEqual,         // 15 >=
    Assign,                // 16 =
    Equals,                // 17 ==
    CommentStart,          // 18 #
    CommentEnd,            // 19 #blabla'EOL'
    EOL,                   // 20 EOL
    Indent,                // 21 EOL....space count > stack top
    Dedent,                // 22 EOL....space count < stack top
    OneQuoteStart,         // 23 "
    TwoQuoteStart,         // 24 ""
    DocumentString,        // 25 """blabla
    OneQuoteEnd,           // 26 """blabla"
    TwoQuoteEnd,           // 27 """blabla""
    DocumentStringEnd,     // 28 """blabla"""
    StringStart,           // 29 '
    StringEnd,             // 30 'blabla'
    IdOrKw,                // 31 string of characters
    Keyword,               // 32 string is a keyword
    Identifier,            // 33 string is an identifier (key for hashtable)
    Integer,               // 34 123
    Double,                // 35 123.456
    AlmostExponential,     // 36 123e / 123.456e
    Exponential,           // 37 123e+/-10 / 123.456e+/-12
    BinOctHex,             // 38 0b / 0o / 0x / 0B / 0O / 0X
    BinaryNum,             // 39 0b1010_1010
    OctalNum,              // 40 0o252
    HexadecimalNum,        // 41 0xAA
    Error = -1,            // -1 lexical error
    ErrorMalloc = -2,      // -2 internal error
    EndOfFile = -3         // -3 endOfFile
} parserState_t;

typedef struct {
    parserState_t tokenType;
    tokenAttribute_t tokenAttribute;
} token_t;

token_t getToken(FILE* in, dynamic_stack_t * indentationStack);

// deciding whether string is a keyword
typedef enum keywords {
    keywordDef,         // def
    keywordIf,          // if
    keywordElse,        // else
    keywordNone,        // None
    keywordPass,        // pass
    keywordReturn,      // return
    keywordWhile,       // while
    keywordInputs,      // inputs()
    keywordInputi,      // inputi()
    keywordInputf,      // inputf()
    keywordPrint,       // print(...)
    keywordLen,         // len(...)
    keywordSubstr,      // substr(...)
    keywordOrd,         // ord(...)
    keywordChar,        // char()
    notKeyword = -1
} keywords_t;

keywords_t isKeyword(const char* string);

parserState_t parserStart(FILE* in, int c, dynamicString_t* actualTokenString);

double strToDouble(const char* string);

int strToInt(const char* string);

int binToDecimal(const char* string);

int octToDecimal(const char* string);

int hexToDecimal(const char* string);

#endif //VUT_FIT_IFJ2019_SCANNER_H
