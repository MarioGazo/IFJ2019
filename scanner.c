// lexical analysis implementation
// Created by Mário Gažo on 2019-10-03.
//

#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

token_t getToken(FILE* in) {
    token_t actualToken;
    actualToken.tokenType = Start;
    actualToken.tokenAttribute.intValue = 0;

    parserState_t state = Start;

    dynamic_stack_t indetationStack;
    stackInit(&indetationStack);

    int spaceCount = 0;

    keywords_t keyword = -1;

    int c;
    while (1) {
        c = getc(in);
        switch (state) {
            case Start: // done
                if (c == '+') {
                    actualToken.tokenType = Plus;          return actualToken; // +
                } else if (c == '-') {
                    actualToken.tokenType = Minus;         return actualToken; // -
                } else if (c == '*') {
                    actualToken.tokenType = Multiply;      return actualToken; // *
                } else if (c == ':') {
                    actualToken.tokenType = Colon;         return actualToken; // :
                } else if (c == '(') {
                    actualToken.tokenType = LeftBracket;   return actualToken; // (
                } else if (c == ')') {
                    actualToken.tokenType = RightBracket;  return actualToken; // )
                } else if (c == ',') {
                    actualToken.tokenType = Comma;         return actualToken; // ,
                } else if (c == EOF) {
                    actualToken.tokenType = EndOfFile;     return actualToken; // EndOfFile
                } else if (c == '/') {
                    state = DivideWRest;           continue; // /
                } else if (c == '!') {
                    state = Exclamation;           continue; // !
                } else if (c == '#') {
                    state = CommentStart;          continue; // #
                } else if (c == '\n') {
                    state = EOL;                   continue; // \n
                } else if (c == '<') {
                    state = Smaller;               continue; // <
                } else if (c == '>') {
                    state = Bigger;                continue; // >
                } else if (c == '=') {
                    state = Assign;                continue; // =
                } else if (c == ' ') {
                    state = Start;                 continue; // ' '
                } else if (c == '\"') {
                    if (dynamicStringInit(actualToken.tokenAttribute.word) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = OneQuoteStart;     continue; // "
                    }
                }  else if (c == '\'') {
                    if (dynamicStringInit(actualToken.tokenAttribute.word) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = StringStart;       continue; // '
                    }
                } else if (isdigit(c) && c != '0') {
                    if (dynamicStringInit(actualToken.tokenAttribute.word) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    }

                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue; // malloc error
                    } else {
                        state = Integer;            continue; // 1-9
                    }
                } else if (c == '0') {
                     if (dynamicStringInit(actualToken.tokenAttribute.word) == false) {
                         state = ErrorMalloc;       continue; // malloc error
                     } else {
                         state = BinOctHex;         continue; // 0
                     }
                } else if (isalpha(c)) {
                    if (dynamicStringInit(actualToken.tokenAttribute.word) == false) {
                        state = ErrorMalloc;        continue; // malloc error
                    }

                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue; // malloc error
                    } else {
                        state = IdOrKw;             continue; // a-zA-Z
                    }
                } else if (c == '_') {
                    if (dynamicStringInit(actualToken.tokenAttribute.word) == false) {
                        state = ErrorMalloc;        continue; // malloc error
                    }

                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue; // malloc error
                    } else {
                        state = Identifier;         continue; // _
                    }
                }  else {
                    ungetc(c,in);
                    state = Error;                  continue; // undefined lexem
                }

            case DivideWRest: // done
                if (c == '/') {
                    actualToken.tokenType = DivideWORest;      return actualToken; // //
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = DivideWRest;       return actualToken; // /
                }

            case Exclamation: // done
                if (c == '=') {
                    actualToken.tokenType = NotEqual;          return actualToken; // !=
                } else {
                    state = Error;                  continue;
                }

            case CommentStart: // done
                if (c == '\n') {
                    state = CommentEnd;             continue;  // #abcdef\n
                } else {
                    continue; // #abcdef
                }

            case CommentEnd: // done
                state = Start;
                continue;

            case EOL: // done
                if (c != ' ') {
                    ungetc(c,in);
                    state = Start;                 continue;
                } else {
                    spaceCount++;
                    while ((c = getc(in)) == ' ') {
                        spaceCount++;
                    }

                    ungetc(c,in);

                    if (c == '\n') {
                        spaceCount = 0;
                        state = Start;             continue;
                    }

                    if (spaceCount == stackTop(indetationStack)) {
                        state = Start;             continue;
                    } else if (spaceCount > stackTop(indetationStack)) {
                        stackPush(&indetationStack, spaceCount);
                        state = Indent;            continue;
                    } else {
                        while (!stackEmpty(&indetationStack)) {
                            actualToken.tokenAttribute.intValue++;

                            if (spaceCount == stackPop(&indetationStack)) {
                                state = Dedent;    break;
                            }
                        }
                        if (stackEmpty(&indetationStack)) {
                            state = Error;         continue;
                        } else {
                            continue;
                        }
                    }
                }

            case Indent: // done                                                    stack:
                ungetc(c,in);                           // def bla():               0
                actualToken.tokenType = Indent;         //     indented code        0 4
                return actualToken;                     //         indented code    0 4 8

            case Dedent: // done
                ungetc(c,in);                           // while true:              0
                actualToken.tokenType = Dedent;         //     code inside loop     0 4
                return actualToken;                     // code outside loop        0

            case OneQuoteStart: // done
                if (c == 34) {
                    state = TwoQuoteStart;         continue; // ""
                } else {
                    state = Error;                 continue;
                }

            case TwoQuoteStart: // done
                if (c == 34) {
                    state = DocumentString;        continue; // """
                } else {
                    state = Error;                 continue;
                }

            case DocumentString: // done
                if (c == '\"') {
                    state = OneQuoteEnd;           continue; // """ abcdefghijklm
                } else {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        continue;
                    }
                }
            case OneQuoteEnd: // done
                if (c == '\"') {
                    state = TwoQuoteEnd;           continue;  // """ abcdefghijklm "
                } else {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,34) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = DocumentString;    continue;  // """ abcdefghijklm " abc
                    }
                }

            case TwoQuoteEnd: // done
                if (c == '\"') {
                    state = DocumentStringEnd;     continue;  // """ abcdefghijklm """
                } else {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = DocumentString;    continue;  // """ abcdefghijklm "" abc
                    }
                }

            case DocumentStringEnd: // done
                ungetc(c,in);
                actualToken.tokenType = DocumentStringEnd; return actualToken;

            case StringStart: // done
                if (c == '\'') {    // 'abc'
                    state = StringEnd;             continue;
                } else if (c < 32) {
                    state = Error;                 continue;
                } else if (c == '\\') {
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
                        case 'x':
                            c  = getc(in) - '0';
                            c *= 16;
                            c += getc(in) - '0';
                        default:
                            ungetc(c,in);
                            c = '\\';
                    }
                }

                if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                    state = ErrorMalloc;
                    continue;
                }
                continue;

            case StringEnd: // done
                ungetc(c,in);
                actualToken.tokenType = StringEnd;             return actualToken;

            case Smaller:
                if (c == '=') {
                    actualToken.tokenType = SmallerOrEqual;    return actualToken; // <=
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Smaller;           return actualToken; // <
                }

            case Bigger: // done
                if (c == '=') {
                    actualToken.tokenType = BiggerOrEqual;     return actualToken; // >=
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Bigger;            return actualToken; // +
                }

            case Assign: // done
                if (c == '=') {
                    actualToken.tokenType = Equals;            return actualToken; // ==
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Assign;            return actualToken; // =
                }

            case IdOrKw: // done
                if (isalpha(c) || isdigit(c) || c == '_') {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    }
                } else {
                    ungetc(c,in);

                    if ((keyword = isKeyword(actualToken.tokenAttribute.word->text)) != -1) {
                        state = Keyword;           continue; // if, else, while
                    }
                    state = Identifier;            continue;
                }

                if (isdigit(c) || c == '_') {
                    state = Identifier;            continue;
                }

                if ((keyword = isKeyword(actualToken.tokenAttribute.word->text)) != -1) {
                    state = Keyword;               continue; // if, else, while
                }

                continue;

            case Identifier: // done
                if (isalpha(c) || isdigit(c) || c == '_') {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    }
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Identifier;        return  actualToken;
                }

            case Keyword: // done
                ungetc(c,in);
                // TODO not sure whether save keyword as enum number, or string
                dynamicStringFree(actualToken.tokenAttribute.word);
                actualToken.tokenAttribute.intValue = keyword;
                actualToken.tokenType = Keyword;               return actualToken;

            case Integer: // done
                if (isdigit(c)) {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;
                        continue;
                    } else {
                        continue;
                    }
                } else if (c == '.') {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;
                        continue;
                    }
                    if (isdigit(c = getc(in))) {
                        ungetc(c,in);
                        state = Double;             continue;
                    } else {
                        ungetc(c,in);
                        dynamicStringFree(actualToken.tokenAttribute.word);
                        state = Error;              continue;
                    }
                } else if (c == 'e' || c == 'E') {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        state = AlmostExponential;  continue;
                    }
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = strToInt(actualToken.tokenAttribute.word->text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case Double: // done
                if (isdigit(c)) {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        continue;
                    }
                } else if (c == 'e' || c == 'E') {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        state = AlmostExponential;  continue;
                    }
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Double;

                    dynamicString_t* StringNumPtr = actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.doubleValue = strToDouble(actualToken.tokenAttribute.word->text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case AlmostExponential: // done
                if (isdigit(c) || c == '+' || c == '-') {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        state = Exponential;        continue;
                    }
                } else {
                    ungetc(c,in);
                    dynamicStringFree(actualToken.tokenAttribute.word);
                    actualToken.tokenType = Error;
                    continue;
                }

            case Exponential: // done
                if (isdigit(c)) {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;        continue;
                    } else {
                        continue;
                    }
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Double; // every exponential num is converted to double

                    dynamicString_t* StringNumPtr = actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.doubleValue = strToDouble(actualToken.tokenAttribute.word->text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case BinOctHex: // done
                if (c == 'b' || c == 'B') {
                    state = BinaryNum;             continue; // 0b 0B
                } else if (c == 'o' || c == 'O') {
                    state = OctalNum;              continue; // 0o 0O
                } else if (c == 'x' || c == 'X') {
                    state = HexadecimalNum;        continue; // 0x 0X
                } else if (isdigit(c)) {
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue; // malloc error
                    } else {
                        state = Integer;           continue; // 0123
                    }
                } else {
                    ungetc(c,in);
                    dynamicStringFree(actualToken.tokenAttribute.word);
                    state = Error;                 continue;
                }

            case BinaryNum: // done
                if (c == '0' || c == '1') { // 0b111
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue;
                    } else {
                        continue;
                    }
                }

                if (c == '_') continue;  // 0b1111_0000

                if (isdigit(c)) {
                    dynamicStringFree(actualToken.tokenAttribute.word);
                    state = Error;                 continue;
                } else {
                    ungetc(c, in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = binToDecimal(actualToken.tokenAttribute.word->text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case OctalNum: // done
                if (c == '0' || c == '1' || c == '2' || c == '3' ||
                    c == '4' || c =='5' || c == '6' || c == '7') { // 0o123
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue;
                    } else {
                        continue;
                    }
                }

                if (c == '_') continue; // 0o123_456

                if (isdigit(c)) {
                    dynamicStringFree(actualToken.tokenAttribute.word);
                    state = Error;                 continue;
                } else {
                    ungetc(c, in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = octToDecimal(actualToken.tokenAttribute.word->text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case HexadecimalNum: // done
                if (isxdigit(c)) { // 0xFFFF
                    if (dynamicStringAddChar(actualToken.tokenAttribute.word,c) == false) {
                        state = ErrorMalloc;       continue;
                    } else {
                        continue;
                    }
                } else if (c == '_') { // 0xFFFF_FFFF
                    continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Integer;

                    dynamicString_t* StringNumPtr = actualToken.tokenAttribute.word;
                    actualToken.tokenAttribute.intValue = hexToDecimal(actualToken.tokenAttribute.word->text);
                    dynamicStringFree(StringNumPtr);
                    return actualToken;
                }

            case Error: // done
                ungetc(c,in);
                actualToken.tokenType = Error;
                return actualToken;

            case ErrorMalloc: // done
                ungetc(c,in);
                actualToken.tokenType = ErrorMalloc;
                return actualToken;
        }
    }

}

keywords_t isKeyword(const char* string) {
    keywords_t keyword;

    if (strcmp(string,"def") == 0) {
        keyword = keywordDef;
    } else if (strcmp(string,"if") == 0) {
        keyword = keywordIf;
    } else if (strcmp(string,"else") == 0) {
        keyword = keywordElse;
    } else if (strcmp(string,"None") == 0) {
        keyword = keywordNone;
    } else if (strcmp(string,"pass") == 0) {
        keyword = keywordPass;
    } else if (strcmp(string,"return") == 0) {
        keyword = keywordReturn;
    } else if (strcmp(string,"while") == 0) {
        keyword = keywordWhile;
    } else if (strcmp(string,"inputs") == 0) {
        keyword = keywordInputs;
    } else if (strcmp(string,"inputi") == 0) {
        keyword = keywordInputi;
    } else if (strcmp(string,"inputf") == 0) {
        keyword = keywordInputf;
    } else if (strcmp(string,"print") == 0) {
        keyword = keywordPrint;
    } else if (strcmp(string,"len") == 0) {
        keyword = keywordLen;
    } else if (strcmp(string,"substr") == 0) {
        keyword = keywordSubstr;
    } else if (strcmp(string,"ord") == 0) {
        keyword = keywordOrd;
    } else if (strcmp(string,"char") == 0) {
        keyword = keywordChar;
    } else {
        keyword = notKeyword;
    }

    return keyword;
}

double strToDouble(const char* string) {
    return strtod(string, NULL);
}

int strToInt(const char* string) {
    return strtol(string,NULL,10);
}
int binToDecimal(const char* string) {
    return strtol(string,NULL,2);
}

int octToDecimal(const char* string) {
    return strtol(string, NULL, 8);
}

int hexToDecimal(const char* string) {
    return strtol(string, NULL, 16);
}