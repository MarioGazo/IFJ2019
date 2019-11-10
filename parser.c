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

#define TRUE 1
#define FALSE 0

// Makro kontolujúce, či je token valid
#define GET_TOKEN \
    if (DEBUG)     \
        printToken(&indentationStack, actualToken); \
    actualToken = getToken(in,&indentationStack);  \
    if (actualToken.tokenType == Error) return LEX_ERR;  \
    if (actualToken.tokenType == ErrorMalloc) return INTERNAL_ERR;  \
    if (actualToken.tokenType == ErrorIndent) return SYNTAX_ERR

// Makro kontolujúce, či je typ tokenu požadovaným typom
#define GET_AND_CHECK_TOKEN(type) \
    GET_TOKEN;  \
    if (actualToken.tokenType != type)  \
        return SYNTAX_ERR

// Globálne premenné
token_t actualToken;
FILE* in;
dynamic_stack_t indentationStack;
// Kód chybového hlásenia prekladača
int errorCode;
// Tabuľka symbolov
hashTable *GlobalTable, *LocalTable;
bool inFunc = false;

// Hlavný program
int analyse(FILE* file) {
    // Inicializácia globálnych premenných
    stackInit(&indentationStack);
    // Rozmery tabuľky prevzaté z internetu
    GlobalTable = TInit(236897);
    LocalTable = TInit(1741);

    // Vstupné dáta zo STDIN
    in = file;

    // Spustenie analýzi
    program();

    // Uvoľnenie pamäti
    TFree(GlobalTable);
    TFree(LocalTable);
    stackFree(&indentationStack);
    return errorCode;
}

// Hlavný mechanizmus spracovávania programu
// Na základe tokenov spracováva telo programu, používateľom definované funkcie
// alebo vstavané funkcie a programové konštrukcie
// Funkcia je opakovane volaná, pokiaľ nie je program spracovaný alebo nie je nájdená chyba
int program() {

    // Zo scanneru získame ďalší token
    GET_TOKEN;

    // Na základe tokenu sa zvolí vetva pre spracovanie
    // Koniec spracovávaného programu
    if (actualToken.tokenType == EndOfFile) {
        return PROG_OK;
    // Používateľom definovaná funkcia
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordDef) { // func def
        inFunc = true;
        if ((errorCode = defFunction()) != PROG_OK) return errorCode;
        return program();
    // DocumentString - je preskočený
    } else if (actualToken.tokenType == DocumentString) {
        return program();
    // Programová konštrukcia, alebo vstavaná funkcia
    } else {
        if ((errorCode = commandList()) != PROG_OK) return errorCode;
        return program();
    }
}

// 1. Vetva - Spracovávanie používateľom definovanej funkcie
int defFunction() {
    // Záznamy funkcie

    hTabItem_t funcRecord; // zaznam funkcie
    hTabItem_t* controlRecord;

    // Prechádzame konštukciu funkcie a kontolujeme syntaktickú správnosť zápisu
    // Vzor: def id ( zoznam_parametrov ) : EOL
    //        sekvencia príkazov
    GET_AND_CHECK_TOKEN(Identifier);                                    // def foo

    funcRecord.key = actualToken.tokenAttribute.word;
    funcRecord.type = TypeFunction;
    funcRecord.defined = TRUE;

    funcRecord.next = NULL;


    GET_AND_CHECK_TOKEN(LeftBracket);                                   // def foo(

    // Počet parametrov
    funcRecord.value.intValue = 0;
    if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;  // def foo(...)

    //Ak uz bola funkcia pouzita, je v HashTable, skontolujeme len pocet parametrov
    if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL){
        // Funkcia uz bola pouzita, skontrolujeme, ci ma rovnaky pocet parametrov
        if (controlRecord->value.intValue != funcRecord.value.intValue){
            return SYNTAX_ERR;
        } else {
            inFunc = false;
            return PROG_OK;
        }
    }

    GET_AND_CHECK_TOKEN(Colon);                                         // def foo(...):
    GET_AND_CHECK_TOKEN(Indent);                                        // def foo(...):

    if ((errorCode = commandList()) != PROG_OK) return errorCode;       // __command_list

    GET_AND_CHECK_TOKEN(Dedent);                                        // end of definition

    // Uložíme funkciu do HashTable
    TInsert(GlobalTable, funcRecord);

    inFunc = false;
    return PROG_OK;
}

// 1a. Vetva - Spracovávanie parametra/parametrov funkcie
int param(hTabItem_t* funcRecord) {
    GET_TOKEN;

    // Spracovávame parametre funkcie a zároveň si ukladáme ich počet
    if (actualToken.tokenType == RightBracket) {            // def foo()
        return PROG_OK;
    } else if (actualToken.tokenType == Identifier) {
        funcRecord->value.intValue++;

        GET_TOKEN;

        // Syntaktická kontrola jednotlivých parametrov
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

// 2. Vetva - Spracovávanie programových konštrukcií a vstavaných príkazov
int commandList() {
    // WHILE
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
    // IF & ELSE
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
    // PRINT
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
    // CHAR
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
    // ORD(S, I)
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
    // SUBSTR(S, I, N)
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
    // LEN(S)
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
    // INPUTI()
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
    // INPUTS()
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
    // INPUTF()
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
    // RETURN
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
    // PASS
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
    // TODO
    } else if (actualToken.tokenType == Identifier) {   // abc....abc = <value> / abc()
        hTabItem_t varRecord, controlToken; // zaznam premennej

        varRecord.key = actualToken.tokenAttribute.word;
        varRecord.defined = TRUE;
        varRecord.next = NULL;

        GET_TOKEN;

        if (actualToken.tokenType == Assign) { // priradenie
            GET_TOKEN;
            if (actualToken.tokenType == Identifier) { //abc = abc...
                GET_TOKEN;
                //Ulozime si identifikator, nevieme ci ide o volanie funkcie alebo vyraz
                controlToken.key = actualToken.tokenAttribute.word;
                controlToken.defined = TRUE;
                controlToken.next = NULL;

                GET_TOKEN;
                //Volanie funkcie
                if (actualToken.tokenType == LeftBracket){ //abc = a(
                    hTabItem_t funcRecord;
                    hTabItem_t* controlRecord;

                    // Musime skontrolovat, ci bola funkcia definovana a ak ano, ci sedi pocet parametrov
                    funcRecord.value.intValue = 0;
                    funcRecord.key = actualToken.tokenAttribute.word;
                    funcRecord.type = TypeFunction;
                    funcRecord.defined = FALSE;
                    funcRecord.next = NULL;

                    if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;  // //abc = a(...)

                    if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL){
                        // Funkcia uz bola definovana, skontrolujeme, ci ma rovnaky pocet parametrov
                        if (controlRecord->value.intValue != funcRecord.value.intValue){
                            return SYNTAX_ERR;
                        }
                    } else {
                        // Uložíme funkciu do HashTable a budeme neskor zistovat jej definiciu
                        TInsert(GlobalTable, funcRecord);
                    }

                } else {
                    // Volanie precedentnej analyzi
                    // Riesi sa vyraz, musime odovzdat dva tokeny
                    // controlToken a actualToken
                    return PROG_OK;
                }
                // Tak ci tak sa jedna o inicializaciu premennej, mozeme ju ulozit do HashTable
                TInsert(GlobalTable, varRecord);
                return PROG_OK;
            } else {
                return SYNTAX_ERR;
            }
        } else { // Vsetko ostatne je chyba
            return SYNTAX_ERR;
        }
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
