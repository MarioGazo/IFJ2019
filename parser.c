/**
 * Implementation of imperative language IFJ2019 compiler
 * @file parser.c
 * @author Mario Gazo (xgazom00)
 * @brief Parser implementation
 */
#include "parser.h"
#include "scanner.h"
#include "code-gen.h"
#include "dynamic-stack.h"
#include "symtable.h"
#include "error.h"

#define DEBUG 1

// kontrola ci je token valid
#define GET_TOKEN \
    if (DEBUG)     \
        printToken(&indentationStack, actualToken); \
    actualToken = getToken(in,&indentationStack);  \
    if (actualToken.tokenType == Error) return LEX_ERR;  \
    if (actualToken.tokenType == ErrorMalloc) return INTERNAL_ERR;  \
    if (actualToken.tokenType == ErrorIndent) return SYNTAX_ERR

// porovnanie tokenu s chcenym typom
#define GET_AND_CHECK_TOKEN(type) \
    GET_TOKEN;  \
    if (actualToken.tokenType != type)  \
        return SYNTAX_ERR

// global variables
token_t actualToken;
FILE* in;
dynamic_stack_t indentationStack;
int errorCode;
hashTable *GlobalTable, *LocalTable;

// spustenie analyzy (init globalnych premennych)
int analyse(FILE* file) {
    stackInit(&indentationStack);
    GlobalTable = TInit(65500); // TODO inu konstantu
    LocalTable = TInit(65000);

    in = file;

    program();

    TFree(GlobalTable);
    TFree(LocalTable);
    stackFree(&indentationStack);
    return errorCode;
}

// telo programu
int program() {

    GET_TOKEN;

    if (actualToken.tokenType == EndOfFile) {
        return PROG_OK;
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordDef) { // func def
        if ((errorCode = defFunction()) != PROG_OK) return errorCode;
        if ((errorCode = program()) != PROG_OK) return errorCode;
    } else if (actualToken.tokenType == DocumentString) {
        if ((errorCode = program()) != PROG_OK) return errorCode;
    } else {
        if ((errorCode = command()) != PROG_OK) return errorCode;
        if ((errorCode = program()) != PROG_OK) return errorCode;
    }

    return SYNTAX_ERR; // ???
}

// v tele programu sa definuje funkcia
int defFunction() {
    hTabItem_t funcRecord;
    GET_AND_CHECK_TOKEN(Identifier);

    funcRecord.key = actualToken.tokenAttribute.word;

    GET_AND_CHECK_TOKEN(LeftBracket);
    GET_AND_CHECK_TOKEN(Identifier);

    if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;

    GET_AND_CHECK_TOKEN(RightBracket);
    GET_AND_CHECK_TOKEN(Colon);
    GET_AND_CHECK_TOKEN(Indent);

    if ((errorCode = commandList()) != PROG_OK) return errorCode;

    GET_AND_CHECK_TOKEN(Dedent);

    TInsert(GlobalTable, funcRecord);
    return PROG_OK;
}

// parameter funkcie
int param(hTabItem_t* funcRecord) {
    GET_TOKEN;

    if (actualToken.tokenType == RightBracket) {
        funcRecord->value.intValue = PROG_OK;
        return 0;
    } else if (actualToken.tokenType == Identifier) {
        funcRecord->value.intValue = 1;

        GET_TOKEN;

        if (actualToken.tokenType == RightBracket) {
            return PROG_OK;
        } else if (actualToken.tokenType == Comma) {
            if ((errorCode = params(funcRecord)) != PROG_OK) return errorCode;
        } else {
            return SYNTAX_ERR;
        }
    } else {
        return SYNTAX_ERR;
    }

    return SYNTAX_ERR;
}

// funkcia ma viacero parametrov
int params(hTabItem_t* funcRecord) {
    GET_AND_CHECK_TOKEN(Identifier);


    while (actualToken.tokenType != RightBracket) {
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Identifier);
        funcRecord->value.intValue++;
    }

    return PROG_OK;
}

// prikazy v tele funkcie, if statementu alebo while cyklu
int commandList() {
    while (actualToken.tokenType != EndOfFile && actualToken.tokenType != Dedent) {
        if ((errorCode = command()) != PROG_OK) return errorCode;
    }

    return PROG_OK;
}

// nasleduje jeden z prikazov
int command() {
    if (actualToken.tokenType == Keyword &&
        actualToken.tokenAttribute.intValue == keywordWhile) {
        return execWhileCycle();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordIf) {
        return execIfStatement();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPrint) {
        return execPrint();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordChar) {
        return execChar();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordOrd) {
        return execOrd();
    } else if (actualToken.tokenType == Keyword &&
                actualToken.tokenAttribute.intValue == keywordSubstr) {
        return execSubstr();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordLen) {
        return execLen();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordChar) {
        return execChar();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputi) {
        return execInputi();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputs) {
        return execInputs();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputf) {
        return execInputf();
    } else if (actualToken.tokenType == Keyword &&
                actualToken.tokenAttribute.intValue == keywordReturn) {
        return execReturn();
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPass) {
        return execPass();
    } else if (actualToken.tokenType == Identifier) {
        // volanie funkcie alebo priradenie identifikatoru
    } else {
        return SYNTAX_ERR;
    }

    return SYNTAX_ERR;
}
