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
bool inFunc = false;    // sme vo funkcii
bool expr = false;      // bol spracovany vyraz

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
    errorCode = program();

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
        // redefinicia funkcie
        if (controlRecord->defined) return SEMPROG_ERR;

        // Funkcia uz bola pouzita, skontrolujeme, ci ma rovnaky pocet parametrov
        if (controlRecord->value.intValue != funcRecord.value.intValue){
            return SEMRUN_ERR;
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

        if (actualToken.tokenType != Colon)                     // while <expr>:
            return SYNTAX_ERR;

        GET_AND_CHECK_TOKEN(Indent);                            // while <expr>:
                                                                // __command_list
        if ((errorCode = commandList()) != PROG_OK) return errorCode;

        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    // IF & ELSE
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordIf) {
        if ((errorCode = expression()) != PROG_OK) return errorCode;  // if <expr>

        if (actualToken.tokenType != Colon)                     // if <expr>:
            return SYNTAX_ERR;

        GET_AND_CHECK_TOKEN(Indent);                            // if <expr>:
                                                                // __command_list
        if ((errorCode = commandList()) != PROG_OK) return errorCode;

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
        } else if (actualToken.tokenType == EOL) {
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
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
    // RETURN
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordReturn) {
        if (!inFunc) return SYNTAX_ERR;

        GET_TOKEN;

        if ((errorCode = expression()) != 0) return errorCode;     // value != None

        // RETURN TMP -> tmp vysledok

        if (expr) { expr = false; } else { GET_TOKEN; }

        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    // PASS
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPass) {
        GET_TOKEN;
        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    // ID
    } else if (actualToken.tokenType == Identifier) {   // abc....abc = <value> / abc()
        hTabItem_t varRecord; // zaznam premennej
        varRecord.key = actualToken.tokenAttribute.word;
        varRecord.defined = TRUE;
        varRecord.next = NULL;

        GET_AND_CHECK_TOKEN(Assign);

        if ((errorCode = assign(&varRecord)) != PROG_OK) return errorCode;
        // Uloženie premennej do HashTable
        if (inFunc) {
            TInsert(LocalTable, varRecord);
        } else {
            TInsert(GlobalTable, varRecord);
        }

        // ak bol priradeny vyraz terminalnym znakom bol bud EOL alebo Dedent
        if (expr) { expr = false; } else { GET_TOKEN; }

        if (actualToken.tokenType == Dedent) {
            return PROG_OK;
        } else if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            return (errorCode = commandList());
        } else {
            return SYNTAX_ERR;
        }
    // ERROR
    } else {
        return SYNTAX_ERR;
    }
}

int term() {
    GET_TOKEN;

    if (actualToken.tokenType == DocumentString) {
        // WRITE DocStr
    } else if (actualToken.tokenType == Identifier) {
        // WRITE id value
    } else if (actualToken.tokenType == String) {
        // WRITE str
    } else if (actualToken.tokenType == Integer) {
        // WRITE int
    } else if (actualToken.tokenType == Double) {
        // WRITE double
    } else if (actualToken.tokenType == Keyword && actualToken.tokenAttribute.intValue == keywordNone) {
        // WRITE None
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

    return SYNTAX_ERR;
}

int assign(hTabItem_t* varRecord) {
    GET_TOKEN;

    // ID
    if (actualToken.tokenType == Identifier) { //abc = abc...
        //Ulozime si identifikator, nevieme ci ide o volanie funkcie alebo vyraz
        hTabItem_t controlToken;
        controlToken.key = actualToken.tokenAttribute.word;
        controlToken.defined = TRUE;
        controlToken.next = NULL;

        GET_TOKEN;
        //Volanie funkcie
        if (actualToken.tokenType == LeftBracket) { //abc = a(
            hTabItem_t funcRecord;
            hTabItem_t *controlRecord;

            // Musime skontrolovat, ci bola funkcia definovana a ak ano, ci sedi pocet parametrov
            funcRecord.value.intValue = 0;
            funcRecord.key = actualToken.tokenAttribute.word;
            funcRecord.type = TypeFunction;
            funcRecord.defined = FALSE;
            funcRecord.next = NULL;

            if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;  // //abc = a(...)

            if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL) {
                // Funkcia uz bola definovana, skontrolujeme, ci ma rovnaky pocet parametrov
                if (controlRecord->value.intValue != funcRecord.value.intValue) {
                    return SEMRUN_ERR;
                }
            } else {
                // Uložíme funkciu do HashTable a budeme neskor zistovat jej definiciu
                TInsert(GlobalTable, funcRecord);
            }

        } else {
            // Volanie precedentnej analyzi
            // Riesi sa vyraz, musime odovzdat dva tokeny
            // controlToken a actualToken
            expr = true;
            if ((errorCode = expression()) != 0) return  errorCode;

            // MOVE var TMP -> tmp vysledok
            return PROG_OK;
        }
        return PROG_OK;
    // EXPRESSION()
    } else if (actualToken.tokenType == Double || actualToken.tokenType == Integer) {
        expr = true;
        if ((errorCode = expression()) != 0) return  errorCode;

        // MOVE var TMP -> tmp vysledok
        return PROG_OK;
    // INPUTF()
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputf) {
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        // READ var float
        varRecord->type = TypeDouble;
        return PROG_OK;
    // INPUTS()
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputs) {
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        // READ var string
        varRecord->type = TypeString;
        return PROG_OK;
    // INPUTI()
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputi) {
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        // READ var int
        varRecord->type = TypeInteger;
        return PROG_OK;
    // LEN(s)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordLen) {
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        long length = actualToken.tokenAttribute.word.capacity;
        GET_AND_CHECK_TOKEN(RightBracket);

        // MOVE var strlen(s)
        varRecord->type = TypeInteger;
        return PROG_OK;
    // SUBSTR(s,i,n)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordSubstr) {
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        dynamicString_t string = actualToken.tokenAttribute.word;
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        int i = actualToken.tokenAttribute.intValue;
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        int n = actualToken.tokenAttribute.intValue;
        GET_AND_CHECK_TOKEN(RightBracket);

        // MOVE VAR s[i:n] (string)
        varRecord->type = TypeString;
        return PROG_OK;
    // ORD(s,i)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordOrd) {
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        dynamicString_t string = actualToken.tokenAttribute.word;
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        int i = actualToken.tokenAttribute.intValue;
        GET_AND_CHECK_TOKEN(RightBracket);

        // MOVE var 's[i]' (int)
        varRecord->type = TypeInteger;
        return PROG_OK;
    // CHR(i)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordChr) {
        int i;
        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(Integer);
        if (actualToken.tokenAttribute.intValue > 255 || actualToken.tokenAttribute.intValue < 0)
            return SEMRUN_ERR;

        i = actualToken.tokenAttribute.intValue;
        GET_AND_CHECK_TOKEN(RightBracket);

        // MOVE var ascii(i) (string)
        varRecord->type = TypeString;
        return PROG_OK;
    // ERROR
    } else {
        return SYNTAX_ERR;
    }
}