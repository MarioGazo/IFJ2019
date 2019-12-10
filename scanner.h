/**
 * Implementation of imperative language IFJ2019 compiler
 * @file scanner.h
 * @author Mario Gazo (xgazom00)
 * @brief Lexical analysis interface
 */
#ifndef VUT_FIT_IFJ2019_SCANNER_H
#define VUT_FIT_IFJ2019_SCANNER_H

#include <stdbool.h>
#include <stdio.h>
#include "dynamic-string.h"
#include "dynamic-stack.h"

/**
 * @union Token attribute is a number or a string
 */
typedef union {
    /** int token value */
    unsigned int intValue;
    /** double token value */
    double doubleValue;
    /** string token value */
    dynamicString_t word;
} tokenAttribute_t;

/**
 * @enum Various states of parser
 */
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
    DocumentStringStart,   // 25 """blabla
    OneQuoteEnd,           // 26 """blabla"
    TwoQuoteEnd,           // 27 """blabla""
    DocumentString,        // 28 """blabla"""
    StringStart,           // 29 '
    String,                // 30 'blabla'
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
    Shift = 100,           // 42 Shift type for prec analy
    Nonterminal = 99,      // 43 Nonterminal for prec analy
    Error = -1,            // -1 lexical error
    ErrorIndent = -2,      // -2 indentation error
    ErrorMalloc = -3,      // -3 internal error
    EndOfFile = -4         // -4 endOfFile
} parserState_t;

/**
 * @struct Token
 *
 * Token represents one of the final states and its potential attribute
 */
typedef struct {
    /** Token type */
    parserState_t tokenType;
    /** Token value */
    tokenAttribute_t tokenAttribute;
} token_t;

/**
 * @brief Lexical analysis finite state machine
 *
 * @param in Source file from which tokens are read
 * @param indentationStack Stack in case of indentation
 * @return Next token
 */
token_t getToken(FILE* in, dynamic_stack_t* indentationStack);

/**
 * @enum Assigning each keyword a numeric value
 */
// deciding whether string is a keyword
typedef enum keywords {
    keywordDef,         //  0 def
    keywordIf,          //  1 if
    keywordElse,        //  2 else
    keywordNone,        //  3 None
    keywordPass,        //  4 pass
    keywordReturn,      //  5 return
    keywordWhile,       //  6 while
    keywordInputs,      //  7 inputs()
    keywordInputi,      //  8 inputi()
    keywordInputf,      //  9 inputf()
    keywordPrint,       // 10 print(...)
    keywordLen,         // 11 len(...)
    keywordSubstr,      // 12 substr(...)
    keywordOrd,         // 13 ord(...)
    keywordChr,        // 14 char(...)
    notKeyword = -1
} keywords_t;

/**
* @defgroup scanner Scanner functions
* Functions for scanning program and turn it intro tokens
* @{
*/

/**
 * @brief Compares string to each keyword
 *
 * @param string String to be compared
 * @return numeric value of the keyword
 */
keywords_t isKeyword(const char* string);

/**
 * @brief Makes it easier to read the finite state machine
 *
 * @param in Source file
 * @param c First read character
 * @param actualTokenString Reference to string attribute
 * @return next state after the first one
 */
parserState_t parserStart(FILE* in, int c, dynamicString_t* actualTokenString);

/**
 * @brief Makes it easier to read
 *
 * @param in Source file
 * @param c Last character
 * @param state Current state of the finite state machine
 * @return Set token
 */
token_t ungetAndSetToken(FILE* in, int c, parserState_t state);

/**
 * @brief Converts string of numbers to its value
 *
 * @param string String to be converted
 * @return Decimal number
 */
double strToDouble(const char* string);

/**
 * @brief Converts string of numbers to its value
 *
 * @param string String to be converted
 * @param base Base of the number
 * @return Whole number
 */
unsigned int strToInt(const char* string, int base);

/**
 * @brief Prints out state for debugging purposes
 *
 * @param indentationStack Dynamic stack to track indentation.
 * @param actualToken Token to be printed out.
 */
void printToken(dynamic_stack_t* indentationStack, token_t actualToken);

/**
 * @}
 */

#endif //VUT_FIT_IFJ2019_SCANNER_H
