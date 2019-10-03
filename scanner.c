// lexical analysis implementation
// Created by Mário Gažo on 2019-10-03.
//

#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

token_t getToken(FILE* in) {
    token_t actualToken;
    actualToken.tokenType = Start;
    actualToken.tokenAtribute.intValue = 0;

    parserState_t state = Start;
    int c;

    while (1) {
        c = getc(in);
        switch (state) {
            case Start:
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
                    actualToken.tokenType = EndOfFile;      return actualToken; // EndOfFile
                } else if (c == '/') {
                    state = DivideWRest;           continue; // /
                } else if (c == '!') {
                    state = Exclamation;           continue; // !
                } else if (c == '#') {
                    state = CommentStart;          continue; // #
                } else if (c == '\n') {
                    state = EOL;                   continue; // \n
                } else if (c == '\"') {
                    state = OneQuoteStart;         continue; // "
                } else if (c == '<') {
                    state = Smaller;               continue; // <
                } else if (c == '>') {
                    state = Bigger;                continue; // >
                } else if (c == '=') {
                    state = Assign;                continue; // =
                } else if (c == ' ') {
                    state = Start;                 continue; // ' '
                } else if (c == '\'') {
                    dynamicStringInit(actualToken.tokenAtribute.word);
                    state = StringStart;
                    continue;
                } else if (isdigit(c) && c != '0') {
                    dynamicStringInit(actualToken.tokenAtribute.word);
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = Integer;
                    continue;
                } else if (c == '0') {
                    dynamicStringInit(actualToken.tokenAtribute.word);
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = BinOctHex;
                    continue;
                } else if (isalpha(c)) {
                    dynamicStringInit(actualToken.tokenAtribute.word);
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = IdOrKw;
                    continue;
                } else if (c == '_') {
                    dynamicStringInit(actualToken.tokenAtribute.word);
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = Identifier;
                    continue;
                }  else {
                    state = Error;                 continue;
                }

            case DivideWRest:
                if (c == '/') {
                    actualToken.tokenType = DivideWORest;      return actualToken;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = DivideWRest;       return actualToken;
                }
            case Exclamation:
                if (c == '=') {
                    actualToken.tokenType = NotEqual;          return actualToken;
                } else {
                    state = Error;                 continue;
                }
            case CommentStart:
                if (c == '\n') {
                    state = CommentEnd;            continue;
                } else {
                    continue;
                }
            case CommentEnd:
                state = Start;
                continue;
            case EOL:
                //TODO
                continue;
            case Indent:
                //TODO
                continue;
            case Dedent:
                //TODO
                continue;
            case OneQuoteStart:
                if (c == '\"') {
                    state = TwoQuoteStart;         continue;
                } else {
                    state = Error;                 continue;
                }
            case TwoQuoteStart:
                if (c == '\"') {
                    state = DocumentString;        continue;
                } else {
                    state = Error;                 continue;
                }
            case DocumentString:
                if (c == '\"') {
                    state = OneQuoteEnd;           continue;
                } else {
                    continue;
                }
            case OneQuoteEnd:
                if (c == '\"') {
                    state = TwoQuoteEnd;           continue;
                } else {
                    state = DocumentString;        continue;
                }
            case TwoQuoteEnd:
                if (c == '\"') {
                    state = DocumentStringEnd;     continue;
                } else {
                    state = DocumentString;        continue;
                }
            case DocumentStringEnd:
                ungetc(c,in);
                state = Start;                     continue;
                // \n and EOF errors TODO
            case StringStart:
                if (c == '\'') {
                    state = StringEnd;             continue;
                } else {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    continue;
                }
            case StringEnd:
                actualToken.tokenType = StringEnd;             return actualToken;
            case Smaller:
                if (c == '=') {
                    actualToken.tokenType = SmallerOrEqual;    return actualToken;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Smaller;           return actualToken;
                }
            case Bigger:
                if (c == '=') {
                    actualToken.tokenType = BiggerOrEqual;     return  actualToken;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Bigger;            return actualToken;
                }
            case Assign:
                if (c == '=') {
                    actualToken.tokenType = Equals;            return actualToken;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Assign;            return actualToken;
                }
            case IdOrKw:
                if (isalpha(c) || isdigit(c) || c == '_') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                } else {
                    ungetc(c,in);
                    if (isKeyword(actualToken.tokenAtribute.word->text) != -1) {
                        state = Keyword;           continue;
                    }
                    state = Identifier;            continue;
                }

                if (isdigit(c) || c == '_') {
                    state = Identifier;            continue;
                }

                if (isKeyword(actualToken.tokenAtribute.word->text) != -1) {
                    state = Keyword;               continue;
                }

                continue;
            case Identifier:
                if (isalpha(c) || isdigit(c) || c == '_') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Identifier;        return  actualToken;
                }
                continue;
            case Keyword:
                ungetc(c,in);
                actualToken.tokenType = Keyword;               return actualToken;
            case Integer:
                if (isdigit(c)) {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c); continue;
                } else if (c == '.') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = Double;                continue;
                } else if (c == 'e' || c == 'E') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = AlmostExponential;     continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Integer;
                    // TODO eval whole num and free
                    return actualToken;
                }
            case Double:
                if (isdigit(c)) {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c); continue;
                } else if (c == 'e' || c == 'E') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = AlmostExponential;
                    continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Double;
                    // TODO eval double num and free
                    return actualToken;
                }
            case AlmostExponential:
                if (isdigit(c) || c == '+' || c == '-') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = Exponential;           continue;
                } else {
                    actualToken.tokenType = Error;
                    dynamicStringFree(actualToken.tokenAtribute.word);
                    return actualToken;
                }
            case Exponential:
                if (isdigit(c)) {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c); continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = Exponential;
                    // TODO eval exponential num and free
                    return actualToken;
                }
            case BinOctHex:
                if (c == 'b' || c == 'B') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = BinaryNum;             continue;
                } else if (c == 'o' || c == 'O') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = OctalNum;              continue;
                } else if (c == 'x' || c == 'X') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = HexadecimalNum;        continue;
                } else if (isdigit(c)) {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);
                    state = Integer;               continue;
                } else {
                    dynamicStringFree(actualToken.tokenAtribute.word);
                    state = Error;                 continue;
                }
            case BinaryNum:
                if (c == '0' || c == '1') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c); continue;
                }

                if (isdigit(c)) {
                    dynamicStringFree(actualToken.tokenAtribute.word);
                    state = Error;                 continue;
                } else {
                    ungetc(c, in);
                    actualToken.tokenType = BinaryNum;
                    //TODO eval and free
                    return actualToken;
                }
            case OctalNum:
                if (c == '0' || c == '1' || c == '2' || c == '3' ||
                    c == '4' || c =='5' || c == '6' || c == '7') {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c); continue;
                }

                if (isdigit(c)) {
                    dynamicStringFree(actualToken.tokenAtribute.word);
                    state = Error;                 continue;
                } else {
                    ungetc(c, in);
                    actualToken.tokenType = OctalNum;
                    //TODO eval and free
                    return actualToken;
                }
            case HexadecimalNum:
                if (isxdigit(c)) {
                    dynamicStringAddChar(actualToken.tokenAtribute.word,c);continue;
                } else {
                    ungetc(c,in);
                    actualToken.tokenType = HexadecimalNum;
                    //TODO eval end free
                    return actualToken;
                }
            case Error:
                ungetc(c,in);
                actualToken.tokenType = Error;
                return actualToken;
        }
    }

}