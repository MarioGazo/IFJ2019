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

#define DEBUG 1 // TODO pred odovzdanim nastavit na 0

#define TRUE 1
#define FALSE 0

// Makro kontolujúce, či je token valid
#define GET_TOKEN \
    actualToken = getToken(in,&indentationStack);  \
    if (DEBUG)     \
        printToken(&indentationStack, actualToken); \
    if (actualToken.tokenType == Error) return LEX_ERR;  \
    if (actualToken.tokenType == ErrorMalloc) return INTERNAL_ERR;  \
    if (actualToken.tokenType == ErrorIndent) return SYNTAX_ERR

// Makro kontolujúce, či je typ tokenu požadovaným typom
#define GET_AND_CHECK_TOKEN(type) \
    GET_TOKEN;  \
    if (actualToken.tokenType != type)  \
        return SYNTAX_ERR

#define PRINT_DEBUG(text) \
    if (DEBUG) \
        printf(text)

// Globálne premenné
token_t actualToken;
FILE* in;
dynamic_stack_t indentationStack;
// Jedinečné hodnoty pre každé náveštie
unsigned int uni_a = 0;
unsigned int uni_b = 42;
// Kód chybového hlásenia prekladača
int errorCode;
// Tabuľka symbolov
hashTable *GlobalTable, *LocalTable;

// Príznaky značia kde sa nachádza čítanie programu
bool inFunc = false;    // sme vo funkcii
bool expr = false;      // bol spracovany vyraz
bool inBody = false;    // prebieha citanie instrukcii v tele hlavneho programu

// Výsledný kód
dynamicString_t code;

// Hlavný program
int analyse(FILE* file) {
    PRINT_DEBUG("Analysis start\n");

    // Inicializácia globálnych premenných
    stackInit(&indentationStack);
    // Rozmery tabuľky prevzaté z internetu
    GlobalTable = TInit(236897);
    LocalTable = TInit(1741);
    // Inicializácia dynamic stringu pre výsledný kód
    dynamicStringInit(&code);

    // Vstupné dáta zo STDIN
    in = file;

    // Nastavime viditelnost vysledneho kodu suboru code-gen.c
    set_code_output(&code);

    if (cg_main_scope_start() == false)     return INTERNAL_ERR;    // Začiatok hlavného rámca
    if (cg_define_b_i_functions() == false) return INTERNAL_ERR;    // Definicia vstavaných funkcii

    // Spustenie analýzi
    errorCode = program();

    if (cg_main_scope_end() == false)       return INTERNAL_ERR;    // Koniec hlavného rámca

    // zistujeme ci boli vsetky volane funkcie definovane
    for (int i = 0; i < 236897; i++) {
        if (GlobalTable->variables[i] != NULL) {
            if (GlobalTable->variables[i]->type == TypeFunction &&
                GlobalTable->variables[i]->defined != TRUE) {
                errorCode = SEMPROG_ERR;
                break;
            }
        }
    }

    // Uvoľnenie pamäti
    TFree(GlobalTable);
    TFree(LocalTable);
    stackFree(&indentationStack);

    return code_write_out(errorCode);
}

// Hlavný mechanizmus spracovávania programu
// Na základe tokenov spracováva telo programu, používateľom definované funkcie
// alebo vstavané funkcie a programové konštrukcie
// Funkcia je opakovane volaná, pokiaľ nie je program spracovaný alebo nie je nájdená chyba
int program() {
    PRINT_DEBUG("Program body\n");

    // Zo scanneru získame ďalší token
    GET_TOKEN;

    // Na základe tokenu sa zvolí vetva pre spracovanie
    // Koniec spracovávaného programu
    if (actualToken.tokenType == EndOfFile) {
        PRINT_DEBUG("End of program\n");

        return PROG_OK;
    // Používateľom definovaná funkcia
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordDef) { // func def
        PRINT_DEBUG("Function definition\n");

        inFunc = true;
        if ((errorCode = defFunction()) != PROG_OK) return errorCode;

        return program();
    // DocumentString - je preskočený
    } else if (actualToken.tokenType == DocumentString) {
        PRINT_DEBUG("DocumentString\n");

        GET_AND_CHECK_TOKEN(EOL);
        return program();
    // Programová konštrukcia alebo vstavaná funkcia
    } else {
        inBody = true;
        if ((errorCode = commandList()) != PROG_OK) return errorCode;
        return program();
    }
}

// 1. Vetva - Spracovávanie používateľom definovanej funkcie
int defFunction() {
    // Prechádzame konštukciu funkcie a kontolujeme syntaktickú správnosť zápisu
    // Vzor: def id ( zoznam_parametrov ) : INDENT
    //          sekvencia príkazov <- EOL po kazdom prikaze
    // DEDENT
    GET_AND_CHECK_TOKEN(Identifier);

    // Hlavička funkcie
    if (cg_fun_start(actualToken.tokenAttribute.word.text) == false) return  INTERNAL_ERR;
    // Návratová hodnota funkcie
    if (cg_fun_retval() == false) return INTERNAL_ERR;
    // Nový rámec
    if (cg_fun_before_params() == false) return INTERNAL_ERR;

    // Záznam funkcie
    hTabItem_t funcRecord;
    funcRecord.key = actualToken.tokenAttribute.word;
    funcRecord.type = TypeFunction;
    funcRecord.defined = TRUE;
    funcRecord.next = NULL;

    GET_AND_CHECK_TOKEN(LeftBracket);

    // Počet parametrov
    funcRecord.value.intValue = 0;
    if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;

    // Ak uz bola funkcia pouzita, je v HashTable, skontolujeme len pocet parametrov
    hTabItem_t* controlRecord;
    if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL){
        // Redefinicia funkcie
        if (controlRecord->defined) return SEMPROG_ERR;

        // Funkcia uz bola pouzita, skontrolujeme, ci ma rovnaky pocet parametrov
        if (controlRecord->value.intValue != funcRecord.value.intValue) {
            return SEMPARAM_ERR;
        }
    }

    GET_AND_CHECK_TOKEN(Colon);
    GET_AND_CHECK_TOKEN(Indent);

    GET_TOKEN;
    if ((errorCode = commandList()) != PROG_OK) return errorCode;

    // Uložíme funkciu do HashTable
    TInsert(GlobalTable, funcRecord);

    // Koniec tela funkcie
    if (cg_fun_end(funcRecord.key.text) == false) return INTERNAL_ERR;
    inFunc = false;
    return PROG_OK;
}

// 1a. Vetva - Spracovávame parametre funkcie a zároveň si ukladáme ich počet
int param(hTabItem_t* funcRecord) {
    PRINT_DEBUG("Parameters\n");

    GET_TOKEN;

    // Funkcia bez parametrov
    switch (actualToken.tokenType) {
        case RightBracket:
            return PROG_OK;

        case Identifier:
            funcRecord->value.intValue++;

            // Definovanie parametrov funkcie v lokálnom rámci jedinečním číselným identifikátorom
            if (cg_fun_param_declare(actualToken.tokenAttribute.word.text) == false) return INTERNAL_ERR;

            GET_TOKEN;

            // Koniec parametrov
            if (actualToken.tokenType == RightBracket) {
                return PROG_OK;
                // Nasleduje daľší parameter
            } else if (actualToken.tokenType == Comma) {
                return param(funcRecord);
            } else {
                return SYNTAX_ERR;
            }

        default:
            return SYNTAX_ERR;
    }
}

// 2. Vetva - Spracovávanie programových konštrukcií a vstavaných príkazov
int commandList() {
    PRINT_DEBUG("Command\n");

    if (actualToken.tokenType == Identifier) {
        PRINT_DEBUG("\tID\n");

        dynamicString_t name = actualToken.tokenAttribute.word;
        token_t controlToken = actualToken; //uloz aktualni token pro pripad ze bude potreba
        GET_TOKEN;

        switch (actualToken.tokenType) {
            case Assign: {
                hTabItem_t varRecord; // zaznam premennej
                varRecord.key = name;
                varRecord.next = NULL;

                // Deklarácia novej premennej
                if (cg_var_declare(varRecord.key.text, inFunc) == false) return INTERNAL_ERR;

                // Aby bola deklarovaná, musí jej byť priradená hodnota
                if ((errorCode = (assign(&varRecord))) != PROG_OK) return errorCode;

                // TODO priradit vysledok do var vo vyslednom kode

                // Uloženie premennej do HashTable
                if (inFunc) {
                    TInsert(LocalTable, varRecord);
                } else {
                    TInsert(GlobalTable, varRecord);
                }

                return (errorCode = commandListContOrEnd());
            }

            case LeftBracket: {
                // Ide o funkciu, ak je definovaná, presvedčíme sa že sedí počet parametrov, inak ju pridáme do hTab
                hTabItem_t funcRecord; // zaznam premennej
                funcRecord.key = name;
                funcRecord.next = NULL;
                funcRecord.value.intValue = 0;

                param(&funcRecord);

                hTabItem_t *controlRecord = NULL;
                if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL) {
                    printf("control: %d func: %d\n",controlRecord->value.intValue,funcRecord.value.intValue);
                    if (controlRecord->value.intValue != funcRecord.value.intValue)
                        return SEMPARAM_ERR;
                } else {
                    funcRecord.type = TypeFunction;
                    funcRecord.defined = FALSE;
                    TInsert(GlobalTable, funcRecord);
                }

                // Skáčeme do tela funkcie
                if (cg_fun_call(funcRecord.key.text) == false) return INTERNAL_ERR;

                return (errorCode = commandListContOrEnd());
            }

            default: {
                PRINT_DEBUG("\tWEXPRESSION\n");

                expr = true;


                // Posielame aktualny a predchádzajúci token
                if ((errorCode = expression(in, &indentationStack, &actualToken, &controlToken, 2)) != 0) return errorCode; //untested

                return (errorCode = commandListContOrEnd());
            }
        }
    } else if (actualToken.tokenType == Double || actualToken.tokenType == Integer || actualToken.tokenType == String) {
        PRINT_DEBUG("\tEXPRESSION\n");

        expr = true;

        // Posielame aktuálny token
        if ((errorCode = expression(in,&indentationStack, &actualToken, NULL, 1)) != 0) return  errorCode;

        return (errorCode = commandListContOrEnd());
    } else if (actualToken.tokenType == Keyword) {
        switch (actualToken.tokenAttribute.intValue) {
            case keywordWhile:
                // Vzor: while (výraz): INDENT
                //          sekvencia prikazov <- EOL po kazdom prikaze
                //       DEDENT
                PRINT_DEBUG("\tWHILE\n");

                // Začiatok while cyklu
                if (cg_while_start(uni_a, uni_b) == false) return INTERNAL_ERR;

                // Podmienka ďaľšej iterácie cyklu, posielame aktuálny token
                if ((errorCode = expression(in, &indentationStack, &actualToken, NULL, 1)) != PROG_OK) return errorCode;

                if (actualToken.tokenType != Colon) return SYNTAX_ERR;
                GET_AND_CHECK_TOKEN(Indent);

                // Zoznam príkazov v tele cyklu
                if ((errorCode = commandList()) != PROG_OK) return errorCode;

                // Koniec while cyklu
                if (cg_while_end(uni_a, uni_b) == false) return INTERNAL_ERR;

                uni_a++;
                uni_b++;
                break;

            case keywordIf:
                // Vzor:  if (výraz): INDENT
                //          sekvencia príkazov <- EOL po kazdom prikaze
                //        DEDENT else: INDENT
                //          sekvencia prikazov <- EOL po kazdom prikaze
                //        DEDENT
                PRINT_DEBUG("\tIF & ELSE\n");

                // Začiatok vetvenia
                cg_if_start(uni_a, uni_b);

                // Podmienka vetvenia, posielame aktuálny token
                GET_TOKEN; // nacti prvni token z vyrazu aby jsme mohli pouzit case 1
                if ((errorCode = expression(in, &indentationStack, &actualToken, NULL, 1)) != PROG_OK) return errorCode; //tested

                if (actualToken.tokenType != Colon) return SYNTAX_ERR;
                GET_AND_CHECK_TOKEN(Indent);

                // Vetva pri splnení podmienky
                if ((errorCode = commandList()) != PROG_OK) return errorCode;

                // Náveštie pri nesplnení podmeinky
                if (cg_if_else_part(uni_a, uni_b) == false) return INTERNAL_ERR;

                GET_AND_CHECK_TOKEN(Keyword);
                if (actualToken.tokenAttribute.intValue != keywordElse) return SYNTAX_ERR;
                GET_AND_CHECK_TOKEN(Colon);
                GET_AND_CHECK_TOKEN(Indent);

                // Vetva pri nesplnení podmienky
                if ((errorCode = commandList()) != PROG_OK) return errorCode;

                // Koniec vetvenia
                if (cg_if_end(uni_a, uni_b) == false) return INTERNAL_ERR;
                uni_a++;
                uni_b++;
                break;

            case keywordPrint:
                PRINT_DEBUG("\tPRINT\n");

                GET_AND_CHECK_TOKEN(LeftBracket);
                PRINT_DEBUG("\t\tTERMS\n");
                // Výpis termov
                if ((errorCode = term()) != PROG_OK) return errorCode;
                break;

            case keywordReturn:
                PRINT_DEBUG("\tRETURN\n");

                // Príraz návratu sa môže nachádzať len v tele funkcie
                if (!inFunc) return SYNTAX_ERR;

                expr = true;

                // Posielame aktuálny token
                GET_TOKEN;//precti prvni token z vyrazu pro aby jsme mohli pouzit case 1 a predat adresu actualToken
                if ((errorCode = expression(in, &indentationStack, &actualToken, NULL, 1)) != 0) return errorCode; //tested

                // TODO RETURN TMP -> tmp vysledok
                // Návrat z tela funkcie
                if (cg_fun_return() == false) return INTERNAL_ERR;
                break;

            case keywordPass:
                PRINT_DEBUG("\tPASS\n");
                // A: TODO PASS
                // Q: Čo sa tú ma vypisovať?
                // A: Asi nič.
                // Q: A sme si istý?
                // A: Nie.
                break;

            case keywordInputf: // input bez priradenia
                PRINT_DEBUG("\tINPUTF\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                //cg_input();
                break;

            case keywordInputs: // input bez priradenia
                PRINT_DEBUG("\tINPUTS\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                //cg_input();
                break;

            case keywordInputi: // input bez priradenia
                PRINT_DEBUG("\tINPUTI\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                //cg_input();
                break;

            case keywordChr:
                PRINT_DEBUG("\tCHR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie chr
                cg_fun_call("FUNCTION_CHR");
                break;

            case keywordLen:
                PRINT_DEBUG("\tLEN\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie len
                if (cg_fun_call("FUNCTION_LEN") == false) return INTERNAL_ERR;
                break;

            case keywordSubstr:
                PRINT_DEBUG("\tSUBSTR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie substr
                if (cg_fun_call("FUNCTION_SUBSTR") == false) return INTERNAL_ERR;
                break;

            case keywordOrd:
                PRINT_DEBUG("\tORD\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie ord
                if (cg_fun_call("FUNCTION_ORD") == false) return INTERNAL_ERR;
                break;

            default:
                return SYNTAX_ERR;
        }
        return (errorCode = commandListContOrEnd());
    } else {
        return SYNTAX_ERR;
    }
}

int term() {
    GET_TOKEN;

    switch (actualToken.tokenType) {
        // Výpis dokumentačného reťazca
        case DocumentString:
            cg_print_literal(actualToken.tokenAttribute.word.text, TypeString);
            break;
        // Výpis hodnoty identifikátora
        case Identifier: {
                hTabItem_t *var;
                // Nachádza sa v globálnej hashT
                if ((var = TSearch(GlobalTable, actualToken.tokenAttribute.word)) != NULL) {
                    if (cg_print_id(var, true) == false) return INTERNAL_ERR;
                    // Nachádza sa v lokálnej hashT
                } else if ((var = TSearch(LocalTable, actualToken.tokenAttribute.word)) != NULL) {
                    if (cg_print_id(var, false) == false) return INTERNAL_ERR;
                    // ID nebol definovaný -> ERROR
                } else {
                    return SEMPROG_ERR; // nedefinovana premenna
                }
            }
            break;
        // Výpis reťazca
        case String:
            if (cg_print_literal(actualToken.tokenAttribute.word.text, TypeString) == false)
                return INTERNAL_ERR;
            break;
        // Výpis celého čísla prevedeného na text
        case Integer: {
            char buffer[100];
            sprintf(buffer, "%d", actualToken.tokenAttribute.intValue);
            if (cg_print_literal(buffer, TypeInteger) == false) return INTERNAL_ERR;
            break;
        }
        // Výpis desatinného čísla prevedeného na text
        case Double: {
            char buffer[100];
            sprintf(buffer, "%a", actualToken.tokenAttribute.doubleValue);
            if (cg_print_literal(buffer, TypeDouble) == false) return INTERNAL_ERR;
            break;
        }
        // Výpis neznámej hodnoty
        case Keyword:
            if (actualToken.tokenAttribute.intValue == keywordNone) {
                if (cg_print_literal("None", TypeNone) == false) return INTERNAL_ERR;
            } else {
                return SYNTAX_ERR;
            }
            break;
        // ERROR
        default:
            return SYNTAX_ERR;
    }

    GET_TOKEN;

    // Končí výpis alebo nasledujú dalšie termy?
    switch (actualToken.tokenType) {
        case RightBracket:
            // Koniec výpisu -> EOL
            if (cg_print_literal("\n", TypeString) == false) return INTERNAL_ERR;
            return PROG_OK;
        case Comma:
            // Ďaľší term -> medzera
            if (cg_print_literal(" ", TypeString) == false)  return INTERNAL_ERR;
            return (errorCode = term());
        default:
            return SYNTAX_ERR;
    }
}

int assign(hTabItem_t* varRecord) {
    PRINT_DEBUG("ASSIGNMENT\n");

    GET_TOKEN;
    token_t controlToken = actualToken; //uloz token pro pripad ze by se mohl hodit
    // ID
    if (actualToken.tokenType == Identifier) { //abc = abc...
        GET_TOKEN;
        //Volanie funkcie
        if (actualToken.tokenType == LeftBracket) { //abc = a(
            PRINT_DEBUG("\tFUNCTION\n");
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
                if (controlRecord->value.intValue != funcRecord.value.intValue)
                    return SEMPARAM_ERR;
            } else {
                // Uložíme funkciu do HashTable a budeme neskor zistovat jej definiciu
                TInsert(GlobalTable, funcRecord);
            }

            // Skáčeme do tela funkcie
            if (cg_fun_call(funcRecord.key.text) == false) return INTERNAL_ERR;

            return PROG_OK;
        } else {
            PRINT_DEBUG("\tEXPRESSION\n");
            expr = true;

            // Volanie precedentnej analyzi
            // Posielame aktuálny a predchádzajúci token
            if ((errorCode = expression(in, &indentationStack, &actualToken, &controlToken, 2)) != 0) return errorCode; //tested

            return PROG_OK;
        }
    } else if (actualToken.tokenType == Double || actualToken.tokenType == Integer) {
        PRINT_DEBUG("\tEXPRESSION\n");

        expr = true;

        // Posielame aktuálny token
        if ((errorCode = expression(in, &indentationStack, &actualToken, NULL, 1)) != 0) return errorCode; //tested

        return PROG_OK;
    } else if (actualToken.tokenType == Keyword) {
        switch (actualToken.tokenAttribute.intValue) {
            case keywordInputf:
                PRINT_DEBUG("\tINPUTF\n");

                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);

                varRecord->type = TypeDouble;
                if (cg_input(*varRecord, inFunc) == false) return INTERNAL_ERR;

                return PROG_OK;

            case keywordInputs:
                PRINT_DEBUG("\tINPUTS\n");

                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);

                varRecord->type = TypeString;
                if (cg_input(*varRecord, inFunc) == false) return INTERNAL_ERR;

                return PROG_OK;


            case keywordInputi:
                PRINT_DEBUG("\tINPUTI\n");

                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);

                varRecord->type = TypeInteger;
                if (cg_input(*varRecord, inFunc) == false) return INTERNAL_ERR;

                return PROG_OK;

            case keywordLen:
                PRINT_DEBUG("\tLEN\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie len
                if (cg_fun_call("FUNCTION_LEN") == false) return INTERNAL_ERR;

                varRecord->type = TypeInteger;
                return PROG_OK;

            case keywordSubstr:
                PRINT_DEBUG("\tSUBSTR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie substr
                if (cg_fun_call("FUNCTION_SUBSTR") == false) return INTERNAL_ERR;

                varRecord->type = TypeString;
                return PROG_OK;

            case keywordOrd:
                PRINT_DEBUG("\tORD\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie ord
                if (cg_fun_call("FUNCTION_ORD") == false) return INTERNAL_ERR;

                varRecord->type = TypeInteger;
                return PROG_OK;

            case keywordChr:
                PRINT_DEBUG("\tCHR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    // TODO gen param
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab();
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        // TODO gen param
                    }
                } else {
                    return SYNTAX_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie chr
                cg_fun_call("FUNCTION_CHR");

                varRecord->type = TypeString;
                return PROG_OK;

            default:
                return SYNTAX_ERR;
        }
    } else {
        return SYNTAX_ERR;
    }
}

int commandListContOrEnd() {
    // Tu sa nič negeneruje
    PRINT_DEBUG("\tAnother command?\n");

    // Pri spracovaní výrazov bol načítaný posledný (terminálny) token
    if (expr) { expr = false; } else { GET_TOKEN; }

    // V tele programu musí byť každý príkaz ukončený koncom riadku,
    // výnimka nastáva len v prípade konca programu
    if (inBody && (actualToken.tokenType == EOL || actualToken.tokenType == EndOfFile)) {
        inBody = false;
        return PROG_OK;
    } else if (inBody) {
        return SYNTAX_ERR;
    }

    // Dedent v sekvencii príkazov zančí koniec, EOL, že nasleduje další prikaz
    if (actualToken.tokenType == Dedent) {
        return PROG_OK;
    } else if (actualToken.tokenType == EOL) {
        GET_TOKEN;
        return (errorCode = commandList());
    } else {
        return SYNTAX_ERR;
    }
}

hTabItem_t* isInLocalOrGlobalhTab() {
    hTabItem_t* varRecord;
    if ((varRecord = (TSearch(LocalTable,actualToken.tokenAttribute.word))) != NULL) {
        return varRecord;
    } else if ((varRecord = (TSearch(GlobalTable,actualToken.tokenAttribute.word))) != NULL) {
        return varRecord;
    } else {
        return NULL;
    }
}
