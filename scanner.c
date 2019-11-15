/**
 * Implementation of imperative language IFJ2019 compiler
 * @file scanner.c
 * @author Mario Gazo (xgazom00)
 * @brief Lexical analysis implementation
 */

#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

token_t getToken(FILE* in, dynamic_stack_t* indentationStack) {

    // initialization of actual token
    token_t actualToken;
    actualToken.tokenType = Start;
    actualToken.tokenAttribute.intValue = 0;

    // state of parser
    parserState_t state = Start;

    // current character
    int c;
    while (1) {
        c = getc(in);
        switch (state) {
            case Start:
                state = parserStart(in,c,&actualToken.tokenAttribute.word);
                continue;

            case Plus:
                return ungetAndSetToken(in,c,Plus); // +

            case Minus:
                return ungetAndSetToken(in,c,Minus); // -

            case Multiply:
                return ungetAndSetToken(in,c,Multiply); // *

            case Colon:
                return ungetAndSetToken(in,c,Colon); // :

            case LeftBracket:
                return ungetAndSetToken(in,c,LeftBracket); // (

            case RightBracket:
                return ungetAndSetToken(in,c,RightBracket); // )

            case Comma:
                return ungetAndSetToken(in,c,Comma); // ,

            case DivideWRest:
                if (c == '/') {
                    state = DivideWORest;           continue;
                } else {
                    ungetAndSetToken(in,c,DivideWRest); // /
                }

            case DivideWORest:
                return ungetAndSetToken(in,c,DivideWORest); // //

            case Exclamation:
                if (c == '=') {
                    state = NotEqual;               continue;
                } else {
                    ungetc(c,in);
                    state = Error;                  continue;
                }

            case NotEqual:
                return ungetAndSetToken(in,c,NotEqual); // !=

            case Smaller:
                if (c == '=') {
                    state = SmallerOrEqual;         continue;
                } else {
                    return ungetAndSetToken(in,c,Smaller); // <
                }

            case SmallerOrEqual:
                return ungetAndSetToken(in,c,SmallerOrEqual); // <=

            case Bigger:
                if (c == '=') {
                    state = BiggerOrEqual;          continue;
                } else {
                    return ungetAndSetToken(in,c,Bigger); // >
                }

            case BiggerOrEqual:
                return ungetAndSetToken(in,c,BiggerOrEqual); // >=

            case Assign:
                if (c == '=') {
                    state = Equals;                 continue;
                } else {
                    return ungetAndSetToken(in,c,Assign); // =
                }

            case Equals:
                return ungetAndSetToken(in,c,Equals); // ==

            case CommentStart:
                if (c == '\n') {
                    ungetc(c,in);
                    state = CommentEnd;             continue;  // #abcdef\n
                } else if (c == EOF) {
                    ungetc(c,in);
                    state = Error;                  continue;
                } else {
                    continue; // #abcdef
                }

            case CommentEnd:
                ungetc(c,in);
                state = Start;
                continue;

            case EOL: {
                // space count in case od indentation
                int spaceCount = 0;

                if (c == ' ') { // counting spaces
                    spaceCount++;
                    while ((c = getc(in)) == ' ')
                        spaceCount++;
                } else {
                    spaceCount = 0;
                }
                ungetc(c, in);

                if (c == '\n') { // skip
                    state = Start;
                    continue;
                }

                if (spaceCount == stackTop(*indentationStack)) { // EOL
                    actualToken.tokenType = EOL;        return actualToken;
                } else if (spaceCount > stackTop(*indentationStack)) { // indent
                    stackPush(indentationStack, spaceCount);
                    state = Indent;
                    continue;
                } else if (!stackEmpty(*indentationStack)) { // dedent
                    stackPop(indentationStack);
                    if (stackTop(*indentationStack) < spaceCount) {
                        state = ErrorIndent;
                        continue;
                    }
                    if (spaceCount == stackTop(*indentationStack)) { // single dedent
                        state = Dedent;
                        continue;
                    } else {  // multiple once
                        for (int i = 0; i < spaceCount; i++)
                            ungetc(' ', in);
                        ungetc('\n', in);

                        state = Dedent;
                        continue;
                    }
                } else { // indentation error
                    state = ErrorIndent;
                    continue;
                }
            }
            case Indent:
                return ungetAndSetToken(in,c,Indent);
                //                        stack:
                // def bla():                   0
                //     indented code            0 4    --> Indent
                //         indented code        0 4 8  --> Indent

            case Dedent:
                return ungetAndSetToken(in,c,Dedent);
                //                       stack:
                // while true:                  0
                //     code inside loop         0 4    --> Indent
                // code outside loop            0      --> Dedent

            case OneQuoteStart:
                if (c == '\"') {
                    state = TwoQuoteStart;         continue; // ""
                } else {
                    state = Error;                 continue;
                }

            case TwoQuoteStart:
                if (c == '\"') {
                    state = DocumentString;        continue; // """
                } else {
                    state = Error;                 continue;
                }

            case DocumentStringStart:
                if (c == '\"') {
                    state = OneQuoteEnd;           continue; // """ abcdefghijklm
                } else if (c == EOF) {
                    ungetc(c,in);
                    state = Error;                  continue;
                } else {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        continue;
                    }
                }
            case OneQuoteEnd:
                if (c == '\"') {
                    state = TwoQuoteEnd;           continue;  // """ abcdefghijklm "
                } else {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,'\"') == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = DocumentString;    continue;  // """ abcdefghijklm " abc
                    }
                }

            case TwoQuoteEnd:
                if (c == '\"') {
                    state = DocumentString;     continue;  // """ abcdefghijklm """
                } else {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = DocumentString;    continue;  // """ abcdefghijklm "" abc
                    }
                }

            case DocumentString:
                ungetc(c,in);
                actualToken.tokenType = DocumentString; return actualToken;

            case StringStart:
                if (c == '\'') {    // 'abc'
                    state = String;             continue;
                } else if (c < 32) {
                    state = Error;                 continue;
                } else if (c == '\\' || (actualToken.tokenAttribute.word.capacity == 1 &&
                                         actualToken.tokenAttribute.word.text[0] == '\\')) {
                    c = getc(in);
                    switch (c) {
                        case '\"':
                            c = '\"';
                            break;
                        case '\'':
                            c = '\'';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        case '\\':
                            c = '\\';
                            break;
                        case 'x': {
                            // two digit hexadecimal number
                            char hexaNum[2];

                            hexaNum[0] = getc(in);
                            if (!isxdigit(hexaNum[0])) {
                                state = Error;      continue;
                            }

                            hexaNum[1] = getc(in);
                            if (!isxdigit(hexaNum[1])) {
                                state = Error;      continue;
                            }

                            c = strToInt(hexaNum, 16);
                            break;
                        }
                        default:
                            if (c < 32) {
                                state = Error;      continue;
                            }
                            ungetc(c,in);
                            c = '\\';
                    }
                }

                if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                    state = ErrorMalloc;            continue;
                }
                continue;

            case String:
                ungetc(c,in);
                actualToken.tokenType = String;             return actualToken;

            case IdOrKw: {
                if (isalpha(c) || isdigit(c) || c == '_') {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    }
                } else {
                    ungetc(c,in);

                    if (isKeyword(actualToken.tokenAttribute.word.text) != -1) {
                        state = Keyword;           continue; // if, else, while
                    }
                    state = Identifier;            continue;
                }

                if (isdigit(c) || c == '_') {
                    state = Identifier;            continue;
                }

                if (isKeyword(actualToken.tokenAttribute.word.text) != -1) {
                    state = Keyword;               continue; // if, else, while
                }

                continue;
            }

            case Identifier:
                if (isalpha(c) || isdigit(c) || c == '_') {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    }
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Identifier;        return  actualToken;
                }

            case Keyword:
                ungetc(c,in);
                actualToken.tokenAttribute.intValue = isKeyword(actualToken.tokenAttribute.word.text);
                actualToken.tokenType = Keyword;               return actualToken;

            case Integer:
                if (isdigit(c)) {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;
                        continue;
                    } else {
                        continue;
                    }
                } else if (c == '.') {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;
                        continue;
                    }
                    if (isdigit(c = getc(in))) {
                        ungetc(c,in);
                        state = Double;             continue;
                    } else {
                        ungetc(c,in);
                        dynamicStringFree(&actualToken.tokenAttribute.word);
                        state = Error;              continue;
                    }
                } else if (c == 'e' || c == 'E') {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        state = AlmostExponential;  continue;
                    }
                } else if (c == '_') {
                    continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = &actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = strToInt(actualToken.tokenAttribute.word.text,10);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case Double:
                if (isdigit(c)) {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        continue;
                    }
                } else if (c == 'e' || c == 'E') {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        state = AlmostExponential;  continue;
                    }
                } else if (c == '_') {
                    continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Double;

                    dynamicString_t* StringNumPtr = &actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.doubleValue = strToDouble(actualToken.tokenAttribute.word.text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case AlmostExponential:
                if (isdigit(c) || c == '+' || c == '-') {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        state = Exponential;        continue;
                    }
                } else {
                    ungetc(c,in);
                    dynamicStringFree(&actualToken.tokenAttribute.word);
                    actualToken.tokenType = Error;
                    continue;
                }

            case Exponential:
                if (isdigit(c)) {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        continue;
                    }
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Double; // every exponential num is converted to double

                    dynamicString_t* StringNumPtr = &actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.doubleValue = strToDouble(actualToken.tokenAttribute.word.text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case BinOctHex:
                if (c == 'b' || c == 'B') {
                    state = BinaryNum;             continue; // 0b 0B
                } else if (c == 'o' || c == 'O') {
                    state = OctalNum;              continue; // 0o 0O
                } else if (c == 'x' || c == 'X') {
                    state = HexadecimalNum;        continue; // 0x 0X
                } else {
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        ungetc(c,in);
                        state = Integer;           continue; // 0123
                    }
                }

            case BinaryNum:
                if (c == '0' || c == '1') { // 0b111
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue;
                    } else {
                        continue;
                    }
                }

                if (c == '_') continue;  // 0b1111_0000

                if (isdigit(c)) {
                    dynamicStringFree(&actualToken.tokenAttribute.word);
                    state = Error;                 continue;
                } else {
                    ungetc(c, in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = &actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = strToInt(actualToken.tokenAttribute.word.text,2);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case OctalNum:
                if (c == '0' || c == '1' || c == '2' || c == '3' ||
                    c == '4' || c =='5' || c == '6' || c == '7') { // 0o123
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue;
                    } else {
                        continue;
                    }
                }

                if (c == '_') continue; // 0o123_456

                if (isdigit(c)) {
                    dynamicStringFree(&actualToken.tokenAttribute.word);
                    state = Error;                 continue;
                } else {
                    ungetc(c, in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = &actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = strToInt(actualToken.tokenAttribute.word.text,8);
                    dynamicStringFree(StringNumPtr);    return actualToken;
                }

            case HexadecimalNum:
                if (isxdigit(c)) { // 0xFFFF
                    if (dynamicStringAddChar(&actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue;
                    } else {
                        continue;
                    }
                } else if (c == '_') { // 0xFFFF_FFFF
                    continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = &actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = strToInt(actualToken.tokenAttribute.word.text,16);
                    dynamicStringFree(StringNumPtr);    return actualToken;
                }

            case Error:
                return ungetAndSetToken(in,c,Error); // lexical error

            case ErrorIndent:
                return ungetAndSetToken(in,c,ErrorIndent); // indentation error

            case ErrorMalloc:
                return ungetAndSetToken(in,c,ErrorMalloc); // memory allocation error

            case EndOfFile:
                ungetc(c,in);
                if (!stackEmpty(*indentationStack)) {
                    stackPop(indentationStack);
                    state = Dedent;                 continue; // Dedent
                } else {
                    actualToken.tokenType = EndOfFile;  return actualToken; // EndOfFile
                }

            default:
                ungetc(c,in);
                state = Start;                      continue;
        }
    }

}

token_t ungetAndSetToken(FILE* in, int c, parserState_t state) {
    ungetc(c,in);
    token_t actualToken;
    actualToken.tokenAttribute.intValue = 0;
    actualToken.tokenType = state;
    return actualToken;
}

parserState_t parserStart(FILE* in, int c, dynamicString_t* actualTokenString) {
    if (c == '+') {             return Plus;
    } else if (c == '-') {      return Minus;
    } else if (c == '*') {      return Multiply;
    } else if (c == ':') {      return Colon;
    } else if (c == '(') {      return LeftBracket;
    } else if (c == ')') {      return RightBracket;
    } else if (c == ',') {      return Comma;
    } else if (c == EOF) {      return EndOfFile;
    } else if (c == '/') {      return DivideWRest;
    } else if (c == '!') {      return Exclamation;
    } else if (c == '#') {      return CommentStart;
    } else if (c == '\n') {     return EOL;
    } else if (c == '<') {      return Smaller;
    } else if (c == '>') {      return Bigger;
    } else if (c == '=') {      return Assign;
    } else if (c == ' ' ||
               c == '\t') {     return Start;
    } else if (c == '\"') {
        if (dynamicStringInit(actualTokenString) == false) {
            return ErrorMalloc;
        } else {                return OneQuoteStart;}
    }  else if (c == '\'') {
        if (dynamicStringInit(actualTokenString) == false) {
            return ErrorMalloc;
        } else {                return StringStart;}
    } else if (isdigit(c) && c != '0') {
        if (dynamicStringInit(actualTokenString) == false) {
            return ErrorMalloc;
        }

        if (dynamicStringAddChar(actualTokenString,c) == false) {
            return ErrorMalloc;
        } else {                return Integer;}
    } else if (c == '0') {
        if (dynamicStringInit(actualTokenString) == false) {
            return ErrorMalloc;
        } else {                return BinOctHex;}
    } else if (isalpha(c)) {
        if (dynamicStringInit(actualTokenString) == false) {
            return ErrorMalloc;
        }

        if (dynamicStringAddChar(actualTokenString,c) == false) {
            return ErrorMalloc;
        } else {                return IdOrKw;}
    } else if (c == '_') {
        if (dynamicStringInit(actualTokenString) == false) {
            return ErrorMalloc;
        }

        if (dynamicStringAddChar(actualTokenString,c) == false) {
            return ErrorMalloc;
        } else {                return Identifier;}
    } else {
        ungetc(c,in);
        return Error;
    }
}

keywords_t isKeyword(const char* string) {
    if (strcmp(string,"def") == 0) {            return keywordDef;
    } else if (strcmp(string,"if") == 0) {      return keywordIf;
    } else if (strcmp(string,"else") == 0) {    return keywordElse;
    } else if (strcmp(string,"None") == 0) {    return keywordNone;
    } else if (strcmp(string,"pass") == 0) {    return keywordPass;
    } else if (strcmp(string,"return") == 0) {  return keywordReturn;
    } else if (strcmp(string,"while") == 0) {   return keywordWhile;
    } else if (strcmp(string,"inputs") == 0) {  return keywordInputs;
    } else if (strcmp(string,"inputi") == 0) {  return keywordInputi;
    } else if (strcmp(string,"inputf") == 0) {  return keywordInputf;
    } else if (strcmp(string,"print") == 0) {   return keywordPrint;
    } else if (strcmp(string,"len") == 0) {     return keywordLen;
    } else if (strcmp(string,"substr") == 0) {  return keywordSubstr;
    } else if (strcmp(string,"ord") == 0) {     return keywordOrd;
    } else if (strcmp(string,"char") == 0) {    return keywordChr;
    } else {                                    return notKeyword;
    }
}

double strToDouble(const char* string) {
    return strtod(string, NULL);
}

int strToInt(const char* string, int base) {
    return strtol(string,NULL,base);
}

void printToken(dynamic_stack_t * indentationStack, token_t actualToken) {
    switch (actualToken.tokenType) {
        case Plus:
            printf("Plus\n");
            break;
        case Minus:
            printf("Minus\n");
            break;
        case Multiply:
            printf("Multiply\n");
            break;
        case Colon:
            printf("Colon\n");
            break;
        case LeftBracket:
            printf("LeftBracket\n");
            break;
        case RightBracket:
            printf("RightBracket\n");
            break;
        case Comma:
            printf("Comma\n");
            break;
        case DivideWORest:
            printf("DivideWORest\n");
            break;
        case DivideWRest:
            printf("DivideWRest\n");
            break;
        case NotEqual:
            printf("NotEqual\n");
            break;
        case Smaller:
            printf("Smaller\n");
            break;
        case SmallerOrEqual:
            printf("SmallerOrEqual\n");
            break;
        case Bigger:
            printf("Bigger\n");
            break;
        case BiggerOrEqual:
            printf("BiggerOrEqual\n");
            break;
        case Assign:
            printf("Assign\n");
            break;
        case Equals:
            printf("Equals\n");
            break;
        case EOL:
            printf("EOL\n");
            break;
        case Indent:
            printf("Indent: ");
            for (int i = 0; i < indentationStack->top + 1; i++) {
                printf("%d ", indentationStack->data[i]);
            }
            printf("\n");
            break;
        case Dedent:
            printf("Dedent: ");
            for (int i = 0; i < indentationStack->top + 1; i++) {
                printf("%d ", indentationStack->data[i]);
            }
            printf("\n");
            break;
        case DocumentString:
            printf("DocumentStringEnd = ");
            printf("%s\n", actualToken.tokenAttribute.word.text);
            break;
        case String:
            printf("String = ");
            printf("%s\n", actualToken.tokenAttribute.word.text);
            break;
        case Identifier:
            printf("Identifier = ");
            printf("%s\n", actualToken.tokenAttribute.word.text);
            break;
        case Keyword:
            printf("Keyword\n");
            break;
        case Integer:
            printf("Integer = ");
            printf("%d\n", actualToken.tokenAttribute.intValue);
            break;
        case Double:
            printf("Double = ");
            printf("%lf\n", actualToken.tokenAttribute.doubleValue);
            break;
        case Error:
            printf("Error\n");
            break;
        case ErrorMalloc:
            printf("ErrorMalloc\n");
            break;
        case EndOfFile:
            printf("EndOfFile\n");
            break;
        default:
            printf("Token value == %i\n",actualToken.tokenType);
            break;
    }
}
