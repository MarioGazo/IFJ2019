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
    dynamicString_t* word;
} tokenAttribute_t;

typedef enum parserState {
    Start,                 // S
    Plus,                  // +
    Minus,                 // -
    Multiply,              // *
    Colon,                 // :
    LeftBracket,           // (
    RightBracket,          // )
    Comma,                 // ,
    DivideWRest,           // /
    DivideWORest,          // //
    Exclamation,           // !
    NotEqual,              // !=
    CommentStart,          // #
    CommentEnd,            // #blabla'EOL'
    EOL,                   // EOL
    Indent,                // EOL....space count > stack top
    Dedent,                // EOL....space count < stack top
    OneQuoteStart,         // "
    TwoQuoteStart,         // ""
    DocumentString,        // """blabla
    OneQuoteEnd,           // """blabla"
    TwoQuoteEnd,           // """blabla""
    DocumentStringEnd,     // """blabla"""
    StringStart,           // '
    StringEnd,             // 'blabla'
    Smaller,               // <
    SmallerOrEqual,        // <=
    Bigger,                // >
    BiggerOrEqual,         // >=
    Assign,                // =
    Equals,                // ==
    IdOrKw,                // string of characters
    Keyword,               // string is a keyword
    Identifier,            // string is an identifier (key for hashtable)
    Integer,               // 123
    Double,                // 123.456
    AlmostExponential,     // 123e / 123.456e
    Exponential,           // 123e+/-10 / 123.456e+/-12
    BinOctHex,             // 0b / 0o / 0x / 0B / 0O / 0X
    BinaryNum,             // 0b1010_1010
    OctalNum,              // 0o252
    HexadecimalNum,        // 0xAA
    Error = -1,            // lexical error
    ErrorMalloc = -2,      // internal error
    EndOfFile = -3         // endOfFile
} parserState_t;

typedef struct {
    parserState_t tokenType;
    tokenAttribute_t tokenAttribute;
} token_t;

token_t getToken(FILE* in);

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

double strToDouble(const char* string);

int strToInt(const char* string);

int binToDecimal(const char* string);

int octToDecimal(const char* string);

int hexToDecimal(const char* string);

#endif //VUT_FIT_IFJ2019_SCANNER_H
