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
#include "expression.h"

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
bool inFunc = false;

// spustenie analyzy (init globalnych premennych)
int analyse(FILE* file) {
    stackInit(&indentationStack);
    GlobalTable = TInit(65500); // TODO inu konstantu
    LocalTable = TInit(65000);

    in = file;

    errorCode = program();

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
        inFunc = true;
        if ((errorCode = defFunction()) != PROG_OK) return errorCode;
        return program();
    } else if (actualToken.tokenType == DocumentString) {
        return program();
    } else {
        if ((errorCode = commandList()) != PROG_OK) return errorCode;
        return program();
    }
}

// v tele programu sa definuje funkcia
int defFunction() {
    hTabItem_t funcRecord; // zaznam funkcie

    GET_AND_CHECK_TOKEN(Identifier);                                    // def foo

    funcRecord.key = actualToken.tokenAttribute.word;

    GET_AND_CHECK_TOKEN(LeftBracket);                                   // def foo(

    funcRecord.value.intValue = 0; // pocet parametrov
    if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;  // def foo(...)

    GET_AND_CHECK_TOKEN(Colon);                                         // def foo(...):
    GET_AND_CHECK_TOKEN(Indent);                                        // def foo(...):
                                                                        // __command_list
    if ((errorCode = commandList()) != PROG_OK) return errorCode;

    GET_AND_CHECK_TOKEN(Dedent);                                        // end of definition
    TInsert(GlobalTable, funcRecord);

    inFunc = false;
    return PROG_OK;
}

// parameter funkcie
int param(hTabItem_t* funcRecord) {
    GET_TOKEN;

    if (actualToken.tokenType == RightBracket) {            // def foo()
        return PROG_OK;
    } else if (actualToken.tokenType == Identifier) {
        funcRecord->value.intValue++;

        GET_TOKEN;

        if (actualToken.tokenType == RightBracket) {        // def foo(a)
            return PROG_OK;
        } else if (actualToken.tokenType == Comma) {        // def foo(a,
            return param(funcRecord);
        } else {
            return SYNTAX_ERR;
        }
    } else {
        return SYNTAX_ERR;
    }
}

// nasleduje jeden z prikazov
int commandList() {
    if (actualToken.tokenType == Keyword &&
        actualToken.tokenAttribute.intValue == keywordWhile) {  // while
        if ((errorCode = expression()) != PROG_OK) return errorCode;  // while <expr>

        GET_AND_CHECK_TOKEN(Colon);                             // while <expr>:
        GET_AND_CHECK_TOKEN(Indent);                            // while <expr>:
                                                                // __command_list
        if ((errorCode = commandList()) != PROG_OK) return errorCode;

        GET_AND_CHECK_TOKEN(Dedent); // koniec while cyklu

        GET_TOKEN;

        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else {
            return (errorCode = commandList());
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordIf) {
        if ((errorCode = expression()) != PROG_OK) return errorCode;  // if <expr>

        GET_AND_CHECK_TOKEN(Colon);                             // if <expr>:
        GET_AND_CHECK_TOKEN(Indent);                            // if <expr>:
                                                                // __command_list
        if ((errorCode = commandList()) != PROG_OK) return errorCode;

        GET_AND_CHECK_TOKEN(Dedent);

        GET_AND_CHECK_TOKEN(Keyword);
        if (actualToken.tokenAttribute.intValue != keywordElse) // if <expr>:
            return SYNTAX_ERR;                                  // __command_list
                                                                // else:
        GET_AND_CHECK_TOKEN(Colon);                             // __command_list
        GET_AND_CHECK_TOKEN(Indent);                            //

        if ((errorCode = commandList()) != PROG_OK) return errorCode;

        GET_AND_CHECK_TOKEN(Dedent);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else {
            return (errorCode = commandList());
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPrint) {
        GET_AND_CHECK_TOKEN(LeftBracket);                       // print(
        if ((errorCode = term()) != PROG_OK) return errorCode;  // print(...)

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordChar) { // char(i)      0 <= i <= 255
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(Integer);
        if (actualToken.tokenAttribute.intValue > 255 || actualToken.tokenAttribute.intValue < 0)
            return SEMRUN_ERR;
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordOrd) { // ord(s,i)
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
                actualToken.tokenAttribute.intValue == keywordSubstr) { // substr(s,i,n)
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordLen) {     // len(s)
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputi) {  // inputi()
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputs) {  // inputs()
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputf) {  // inputf()
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Keyword &&
                actualToken.tokenAttribute.intValue == keywordReturn) { // return <value>
        if (!inFunc) return SYNTAX_ERR;

        GET_TOKEN;

        if (actualToken.tokenType == Dedent) { // value == None
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            if ((errorCode = expression()) != 0) return errorCode;     // value != None

            GET_TOKEN;
            if (actualToken.tokenType == Dedent) {
                return PROG_OK;
            } else if (actualToken.tokenType == EOL) {
                GET_TOKEN;
                return (errorCode = commandList());
            } else {
                return SYNTAX_ERR;
            }
        }
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPass) { // pass
        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    } else if (actualToken.tokenType == Identifier) {   // abc....abc = <value> / abc()
        return PROG_OK;// volanie funkcie alebo priradenie identifikatoru
    } else {
        return SYNTAX_ERR;
    }
}

int term() {
    GET_TOKEN;

    if (actualToken.tokenType == DocumentString) {
        // docStr
    } else if (actualToken.tokenType == Identifier) {
        // id value
    } else if (actualToken.tokenType == String) {
        // str
    } else if (actualToken.tokenType == Integer) {
        // int
    } else if (actualToken.tokenType == Double) {
        // double
    } else if (actualToken.tokenType == Keyword && actualToken.tokenAttribute.intValue == keywordNone) {
        // None
    } else {
        return SYNTAX_ERR;
    }

    GET_TOKEN;

    if (actualToken.tokenType == RightBracket) {
        PROG_OK;
    } else if (actualToken.tokenType == Comma) {
        return (errorCode = term());
    } else {
        return SYNTAX_ERR;
    }
}
