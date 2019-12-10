/**
 * Implementation of imperative language IFJ2019 compiler
 * @file parser.c
 * @author Mario Gazo (xgazom00), Juraj Lazur (xlazur00)
 * @brief Parser implementation
 */
#include "parser.h"
#include "scanner.h"
#include "code-gen.h"
#include "dynamic-stack.h"
#include "symtable.h"
#include "error.h"
#include "expression.h"

// DEBUG - kontrolné výpisy celého programu
#define DEBUG 0

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
int ret_type = 0;

// Jedinečné hodnoty pre každé náveštie
unsigned int uni_a = 0;
unsigned int uni_b = 42;
unsigned int uni = 0;

// Zásobníky pre vnorené funkcie
dynamic_stack_t unistack_if;
dynamic_stack_t unistack_while;

// Kód chybového hlásenia prekladača
int errorCode;
int unsigned main_parts = 0;
int num_of_l = 0;

// Tabuľka symbolov - globálna, lokálna
hashTable *GlobalTable, *LocalTable;

// Príznaky značia kde sa nachádza čítanie programu
bool inFunc = false;    // sme vo funkcii
bool inRecursion = false;    // sme vo funkcii
bool expr = false;      // bol spracovany vyraz
bool while_in = false;  // sme vo funkcii while
bool if_in = false; // sme vo funkcii if

// Výsledný kód
dynamicString_t code;

dynamicString_t while_def;
dynamicString_t if_def;

// Hlavný program
int analyse(FILE* file) {
    PRINT_DEBUG("Analysis start\n");

    // Inicializácia globálnych premenných
    stackInit(&indentationStack);
    stackInit(&unistack_if);
    stackInit(&unistack_while);
    // Rozmery tabuľky prevzaté z internetu
    GlobalTable = TInit(236897);
    LocalTable = TInit(1741);
    // Inicializácia dynamic stringu pre výsledný kód
    dynamicStringInit(&code);

    // Vstupné dáta zo STDIN
    in = file;

    // Nastavime viditelnost vysledneho kodu suboru code-gen.c
    set_code_output(&code, GlobalTable);

    if (cg_code_header() == false)          return INTERNAL_ERR;    // Hlavicka vystupneho kodu
    if (cg_define_b_i_functions() == false) return INTERNAL_ERR;    // Definicia vstavaných funkcii
    if (cg_main_scope() == false)           return INTERNAL_ERR;    // Yaciatok hlavneho ramca

    // Spustenie analýzi
    errorCode = program();

    if (cg_code_footer() == false)          return INTERNAL_ERR;    // Koniec hlavného rámca

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
    stackFree(&unistack_if);
    stackFree(&unistack_while);

    return code_write_out(errorCode);
}

// Hlavný mechanizmus spracovávania programu
// Na základe tokenov spracováva telo programu, používateľom definované funkcie
// alebo vstavané funkcie a programové konštrukcie
// Funkcia je opakovane volaná, pokiaľ nie je program spracovaný alebo nie je nájdená chyba
int program() {
    PRINT_DEBUG("Program body\n");

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

        main_parts++;
        ADD_CODE("JUMP $$main"); ADD_CODE_INT(main_parts); ADD_CODE("\n");

        inFunc = true;
        if ((errorCode = defFunction()) != PROG_OK) return errorCode;

        return program();
    // Programová konštrukcia alebo funkcia
    } else {

        if ((errorCode = command()) != PROG_OK) return errorCode;

        // If alebo while, nemozem ocakavat EOL, nasledujuci token uz bude dalsi prikaz alebo EOF
        if (actualToken.tokenType == Dedent && !expr) {
            return program();
        }

        // Expression
        if (expr) { expr = false; } else { GET_TOKEN; }

        // Konci zadavanie prikazov
        if (actualToken.tokenType == EndOfFile) {
            return PROG_OK;
        // Nasleduje dalsi prikaz
        } else if (actualToken.tokenType == EOL || actualToken.tokenType == Dedent) {
            return program();
        // Nespravne ukonceny prikaz
        } else {
            return SYNTAX_ERR;
        }
    }
}

// 1. Vetva - Spracovávanie používateľom definovanej funkcie
int defFunction() {
    // Prechádzame konštukciu funkcie a kontolujeme syntaktickú správnosť zápisu
    // Vzor: def id ( zoznam_parametrov ) : INDENT
    //          sekvencia príkazov <- EOL po kazdom prikaze
    // DEDENT
    GET_AND_CHECK_TOKEN(Identifier);

    // Hlavička funkcie + novy ramec
    if (cg_fun_start(actualToken.tokenAttribute.word.text) == false) return  INTERNAL_ERR;

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

        controlRecord->defined = TRUE;
    }

    // Uložíme funkciu do HashTable
    TInsert(GlobalTable, funcRecord);

    GET_AND_CHECK_TOKEN(Colon);
    GET_AND_CHECK_TOKEN(Indent);

    // Command list musi mat aspon 1 command
    GET_TOKEN;
    do {
        if ((errorCode = command()) != PROG_OK) return errorCode;

        // Command skoncil dedentom, islo o if alebo while
        if (actualToken.tokenType == Dedent && !expr) {
            GET_TOKEN;
            continue;
        // V pripade ze slo o Dedent ukoncujucim Command list funkcie,
        // islo o nacitanie terminalneho tokenu precedentnou analyzou
        } else if (actualToken.tokenType == Dedent) {
            break;
        }

        // V pripade nevolania precedentnej analyze je treba nacitat dalsi token, nebol nacitany ako terminalny
        if (expr) { expr = false; } else { GET_TOKEN; }

        // Nasleduje dalsi prikaz
        if (actualToken.tokenType == EOL) {
            GET_TOKEN;
            continue;
            // Koniec command listu
        } else if (actualToken.tokenType == Dedent) {
            break;
            // Nespravne ukonceny prikaz
        } else {
            return SYNTAX_ERR;
        }
    } while (actualToken.tokenType != Dedent);
    expr = false;
    // Koniec Command listu, plati ze actualToken.tokenType == Dedent, expr == false

    // Koniec tela funkcie
    if (cg_fun_end(funcRecord.key.text) == false) return INTERNAL_ERR;

    // Reinicializacia lokalnej tabulky
    TFree(LocalTable);
    LocalTable = TInit(1741);

    ADD_CODE("LABEL $$main"); ADD_CODE_INT(main_parts); ADD_CODE("\n");

    inFunc = false;
    return PROG_OK;
}

// 1a. Vetva - Spracovávame parametre funkcie a zároveň si ukladáme ich počet
int param(hTabItem_t* funcRecord) {
    PRINT_DEBUG("Parameters\n");

    GET_TOKEN;
    if (!inRecursion) {
        // Definicia parametrov
        switch (actualToken.tokenType) {
            case RightBracket:
                return PROG_OK;

            case Identifier:
                funcRecord->value.intValue++;
                if (cg_fun_param_declare(actualToken.tokenAttribute.word.text, funcRecord->value.intValue) == false)return INTERNAL_ERR;
                break;

            default:
                return SYNTAX_ERR;
        }
    } else {
        // Priradenie hodnot parametrom
        switch (actualToken.tokenType) {
            case RightBracket:
                return PROG_OK;

            case Identifier:
                funcRecord->value.intValue++;
                if (cg_fun_param_assign(funcRecord->value.intValue,actualToken,inFunc) == false)
                    return INTERNAL_ERR;
                break;

            case Keyword:
                if (actualToken.tokenAttribute.intValue != keywordNone) return SYNTAX_ERR;
                break;
            case String:
            case Integer:
            case Double:
            case DocumentString:
                funcRecord->value.intValue++;
                if ((cg_fun_param_assign(funcRecord->value.intValue,actualToken, inFunc)) == false)
                    return INTERNAL_ERR;
                break;
            default:
                return SYNTAX_ERR;
        }
    }

    // Koniec parametrov?
    GET_TOKEN;
    if (actualToken.tokenType == RightBracket) {
        return PROG_OK;
        // Nasleduje daľší parameter
    } else if (actualToken.tokenType == Comma) {
        return param(funcRecord);
    } else {
        return SYNTAX_ERR;
    }
}

// 2. Vetva - Spracovávanie programových konštrukcií a vstavaných príkazov
int command() {
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

                hTabItem_t* controlRecord = isInLocalOrGlobalhTab(varRecord.key);
                // Priradenie hodnoty do nexistujucej premennej
                if (!controlRecord) {
                    if (while_in){
                        if (cg_var_declare(varRecord.key.text, &while_def, inFunc) == false)              return INTERNAL_ERR;
                    } else if (if_in){
                        if (cg_var_declare(varRecord.key.text, &if_def, inFunc) == false)              return INTERNAL_ERR;
                    } else {
                        if (cg_var_declare(varRecord.key.text, &code, inFunc) == false)              return INTERNAL_ERR;
                    }

                    if ((errorCode = (assign(&varRecord))) != PROG_OK) return errorCode; // Vyraz na priradenie

                    if (cg_assign_expr_result(varRecord.key.text, inFunc) == false)    return INTERNAL_ERR;

                    // Uloženie novej premennej do HashTable
                    if (inFunc) {
                        TInsert(LocalTable, varRecord);
                    } else {
                        TInsert(GlobalTable, varRecord);
                    }
                // Priradenie hodnoty do existujucej premennej
                } else {
                    // Nemozme priradit funkcii
                    if (controlRecord->type == TypeFunction)
                        return SEMPROG_ERR;

                    if ((errorCode = (assign(&varRecord))) != PROG_OK) return errorCode; // Vyraz na priradenie

                    if (cg_assign_expr_result(varRecord.key.text, inFunc) == false)    return INTERNAL_ERR;

                    controlRecord->type = varRecord.type;
                    controlRecord->value = varRecord.value;
                }

                return PROG_OK;
            }

            case LeftBracket: {
                // Ide o funkciu, ak je definovaná, presvedčíme sa že sedí počet parametrov, inak ju pridáme do hTab
                hTabItem_t funcRecord; // zaznam funkcie
                funcRecord.key = name;
                funcRecord.next = NULL;
                funcRecord.value.intValue = 0;

                hTabItem_t *controlRecord = NULL;

                if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL){
                    inRecursion = true;
                }

                ADD_INST("CREATEFRAME");
                param(&funcRecord);

                inRecursion = false;

                if (controlRecord != NULL) {
                    if (controlRecord->value.intValue != funcRecord.value.intValue)
                        return SEMPARAM_ERR;
                } else {
                    funcRecord.type = TypeFunction;
                    funcRecord.defined = FALSE;
                    TInsert(GlobalTable, funcRecord);
                }

                // Skáčeme do tela funkcie
                if (cg_fun_call(funcRecord.key.text) == false) return INTERNAL_ERR;

                return PROG_OK;
            }

            default: { // Vyhodnoti sa expression no nikam sa neprirade
                PRINT_DEBUG("\tEXPRESSION\n");

                expr = true;

                // Posielame aktualny a predchádzajúci token
                if ((errorCode = expression(in, &indentationStack, &actualToken, &controlToken, 2, &ret_type, inFunc)) != 0) return errorCode; //untested

                return PROG_OK;
            }
        }
    } else if (actualToken.tokenType == Double || actualToken.tokenType == Integer ||
               actualToken.tokenType == DocumentString || actualToken.tokenType == String ||
               actualToken.tokenType == RightBracket) {
        PRINT_DEBUG("\tEXPRESSION\n");

        expr = true;
        // Posielame aktuálny token
        if ((errorCode = expression(in,&indentationStack, &actualToken, NULL, 1, &ret_type, inFunc)) != PROG_OK) return  errorCode;

        return PROG_OK;
    } else if (actualToken.tokenType == Keyword) {
        switch (actualToken.tokenAttribute.intValue) {
            case keywordWhile:
                // Vzor: while (výraz): INDENT
                //          sekvencia prikazov <- EOL po kazdom prikaze
                //       DEDENT
                PRINT_DEBUG("\tWHILE\n");

                uni_a++;        uni_b++;

                uni = uni_a + uni_b;

                stackPush(&unistack_while, uni);

                ADD_CODE("\n");
                ADD_INST("# Begin of while");
                if (!while_in){
                    dynamicStringInit(&while_def);
                    while_in = true;

                    ADD_CODE("JUMP $WHILE_PARAMS"); ADD_CODE_INT(uni); ADD_CODE("\n");
                    if (!cg_label("WHILE_PARAMS_RET", "", uni)) return false;
                }

                if (!cg_label("WHILE", "", uni)) return false;

                // Podmienka ďaľšej iterácie cyklu, posielame aktuálny token
                GET_TOKEN;
                if ((errorCode = expression(in, &indentationStack,
                        &actualToken, NULL, 1, &ret_type, inFunc)) != PROG_OK)
                    return errorCode;

                if (actualToken.tokenType != Colon) return SYNTAX_ERR;
                GET_AND_CHECK_TOKEN(Indent);

                if (cg_while_start(uni) == false) return INTERNAL_ERR;

                // Command list musi mat aspon 1 command
                GET_TOKEN;
                do {
                    if ((errorCode = command()) != PROG_OK) return errorCode;

                    // Command skoncil dedentom, islo o if alebo while
                    if (actualToken.tokenType == Dedent && !expr) {
                        GET_TOKEN;
                        continue;
                        // V pripade ze slo o Dedent ukoncujucim Command list funkcie,
                        // islo o nacitanie terminalneho tokenu precedentnou analyzou
                    } else if (actualToken.tokenType == Dedent) {
                        break;
                    }

                    // V pripade nevolania precedentnej analyze je treba nacitat dalsi token, nebol nacitany ako terminalny
                    if (expr) { expr = false; } else { GET_TOKEN; }

                    // Nasleduje dalsi prikaz
                    if (actualToken.tokenType == EOL) {
                        GET_TOKEN;
                        continue;
                        // Koniec command listu
                    } else if (actualToken.tokenType == Dedent) {
                        break;
                        // Nespravne ukonceny prikaz
                    } else {
                        return SYNTAX_ERR;
                    }
                } while (actualToken.tokenType != Dedent);
                expr = false;
                // Koniec Command listu, plati ze actualToken.tokenType == Dedent, expr == false

                // Koniec while cyklu
                uni = stackTop(unistack_while);
                if (cg_while_end(uni) == false) return INTERNAL_ERR;

                if (while_in) {
                    while_in = false;
                    if (!cg_label("WHILE_PARAMS", "", uni)) return false;
                    dynamicStringAddString(&code, while_def.text);
                    ADD_CODE("JUMP $WHILE_PARAMS_RET"); ADD_CODE_INT(uni); ADD_CODE("\n");
                    dynamicStringFree(&while_def);
                }

                if (!cg_label("WHILE", "A", uni)) return false;
                ADD_INST("# End of while");
                stackPop(&unistack_while);

                return PROG_OK;

            case keywordIf:
                // Vzor:  if (výraz): INDENT
                //          sekvencia príkazov <- EOL po kazdom prikaze
                //        DEDENT else: INDENT
                //          sekvencia prikazov <- EOL po kazdom prikaze
                //        DEDENT
                PRINT_DEBUG("\tIF & ELSE\n");

                ADD_INST("\n# Begin of If Then");
                uni_a++;            uni_b++;
                num_of_l ++;

                uni = uni_a + uni_b;

                if (!if_in){
                    dynamicStringInit(&if_def);
                    if_in = true;

                    ADD_CODE("JUMP $IF_PARAMS"); ADD_CODE_INT(uni); ADD_CODE("\n");
                    if (!cg_label("IF_PARAMS_RET", "", uni)) return false;
                }

                // Podmienka vetvenia, posielame aktuálny token
                GET_TOKEN; // nacti prvni token z vyrazu aby jsme mohli pouzit case 1
                if ((errorCode = expression(in, &indentationStack,
                        &actualToken, NULL, 1, &ret_type, inFunc)) != PROG_OK)
                            return errorCode;

                if (actualToken.tokenType != Colon) return SYNTAX_ERR;

                GET_AND_CHECK_TOKEN(Indent);

                // Začiatok vetvenia
                cg_if_start(uni);

                stackPush(&unistack_if, uni);

                // Command list musi mat aspon 1 command
                GET_TOKEN;
                do {
                    if ((errorCode = command()) != PROG_OK) return errorCode;

                    // Command skoncil dedentom, islo o if alebo while
                    if (actualToken.tokenType == Dedent && !expr) {
                        GET_TOKEN;
                        continue;
                        // V pripade ze slo o Dedent ukoncujucim Command list funkcie,
                        // islo o nacitanie terminalneho tokenu precedentnou analyzou
                    } else if (actualToken.tokenType == Dedent) {
                        break;
                    }

                    // V pripade nevolania precedentnej analyze je treba nacitat dalsi token, nebol nacitany ako terminalny
                    if (expr) { expr = false; } else { GET_TOKEN; }

                    // Nasleduje dalsi prikaz
                    if (actualToken.tokenType == EOL) {
                        GET_TOKEN;
                        continue;
                    // Koniec command listu
                    } else if (actualToken.tokenType == Dedent) {
                        break;
                    // Nespravne ukonceny prikaz
                    } else {
                        return SYNTAX_ERR;
                    }
                } while (actualToken.tokenType != Dedent);
                expr = false;
                // Koniec Command listu, plati ze actualToken.tokenType == Dedent, expr == false

                uni = stackTop(unistack_if);
                // Náveštie pri nesplnení podmeinky
                if (cg_if_else_part(uni) == false) return INTERNAL_ERR;

                GET_AND_CHECK_TOKEN(Keyword);
                if (actualToken.tokenAttribute.intValue != keywordElse) return SYNTAX_ERR;
                GET_AND_CHECK_TOKEN(Colon);
                GET_AND_CHECK_TOKEN(Indent);

                // Command list musi mat aspon 1 command
                GET_TOKEN;
                do {
                    if ((errorCode = command()) != PROG_OK) return errorCode;

                    // Command skoncil dedentom, islo o if alebo while
                    if (actualToken.tokenType == Dedent && !expr) {
                        GET_TOKEN;
                        continue;
                        // V pripade ze slo o Dedent ukoncujucim Command list funkcie,
                        // islo o nacitanie terminalneho tokenu precedentnou analyzou
                    } else if (actualToken.tokenType == Dedent) {
                        break;
                    }

                    // V pripade nevolania precedentnej analyze je treba nacitat dalsi token, nebol nacitany ako terminalny
                    if (expr) { expr = false; } else { GET_TOKEN; }

                    // Nasleduje dalsi prikaz
                    if (actualToken.tokenType == EOL) {
                        GET_TOKEN;
                        continue;
                        // Koniec command listu
                    } else if (actualToken.tokenType == Dedent) {
                        break;
                        // Nespravne ukonceny prikaz
                    } else {
                        return SYNTAX_ERR;
                    }
                } while (actualToken.tokenType != Dedent);
                expr = false;
                // Koniec Command listu, plati ze actualToken.tokenType == Dedent, expr == false

                uni = stackTop(unistack_if);
                ADD_CODE("JUMP $"); ADD_CODE("IF"); ADD_CODE("A"); ADD_CODE_INT(uni); ADD_CODE("\n");
                // Koniec vetvenia

                if (if_in) {
                    if_in = false;
                    if (!cg_label("IF_PARAMS", "", uni)) return false;
                    dynamicStringAddString(&code, if_def.text);
                    ADD_CODE("JUMP $IF_PARAMS_RET"); ADD_CODE_INT(uni); ADD_CODE("\n");
                    dynamicStringFree(&if_def);
                }

                if (cg_if_end(uni) == false) return INTERNAL_ERR;

                stackPop(&unistack_if);
                return PROG_OK;

            case keywordPrint:
                PRINT_DEBUG("\tPRINT\n");

                GET_AND_CHECK_TOKEN(LeftBracket);
                PRINT_DEBUG("\t\tTERMS\n");

                // Výpis termov
                if ((errorCode = term()) != PROG_OK) return errorCode;
                return PROG_OK;

            case keywordReturn:
                PRINT_DEBUG("\tRETURN\n");

                // Príraz návratu sa môže nachádzať len v tele funkcie
                if (!inFunc) return SYNTAX_ERR;

                expr = true;
                // Posielame aktuálny token
                GET_TOKEN;//precti prvni token z vyrazu pro aby jsme mohli pouzit case 1 a predat adresu actualToken
                if ((errorCode = expression(in, &indentationStack, &actualToken, NULL, 1, &ret_type, inFunc)) != 0) return errorCode; //tested

                // Návrat z tela funkcie
                if (cg_fun_return() == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordPass:
                PRINT_DEBUG("\tPASS\n");
                return PROG_OK;

            case keywordInputf: // input bez priradenia
                PRINT_DEBUG("\tINPUTF\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                if (cg_input(TypeDouble) == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordInputs: // input bez priradenia
                PRINT_DEBUG("\tINPUTS\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                if (cg_input(TypeString) == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordInputi: // input bez priradenia
                PRINT_DEBUG("\tINPUTI\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                if (cg_input(TypeInteger) == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordChr:
                PRINT_DEBUG("\tCHR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    if (actualToken.tokenAttribute.intValue > 255)
                        return SEMRUN_ERR;
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie chr
                cg_fun_call("function_len");
                return PROG_OK;

            case keywordLen:
                PRINT_DEBUG("\tLEN\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie len
                if (cg_fun_call("function_len") == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordSubstr:
                PRINT_DEBUG("\tSUBSTR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    cg_fun_param_assign(1, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(1, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    cg_fun_param_assign(2, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(2, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie substr
                if (cg_fun_call("function_substr") == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordOrd:
                PRINT_DEBUG("\tORD\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    cg_fun_param_assign(1, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(1, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie ord
                if (cg_fun_call("function_ord") == false) return INTERNAL_ERR;
                return PROG_OK;

            default:
                return SYNTAX_ERR;
        }
    } else {
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
            funcRecord.key = controlToken.tokenAttribute.word;
            funcRecord.type = TypeFunction;
            funcRecord.defined = FALSE;
            funcRecord.next = NULL;

            if ((controlRecord = TSearch(GlobalTable, funcRecord.key)) != NULL){
                inRecursion = true;
            }

            ADD_INST("CREATEFRAME");
            if ((errorCode = param(&funcRecord)) != PROG_OK) return errorCode;  // //abc = a(...)

            inRecursion = false;

            if (controlRecord != NULL) {
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
            // Posielame aktuálny a predchádzajúci token
            if ((errorCode = expression(in, &indentationStack,
                    &actualToken, &controlToken, 2, &ret_type, inFunc)) != 0) return errorCode;

            switch (ret_type) {
                case String:
                case Identifier: varRecord->type = TypeString;
                    break;
                case Integer:
                    varRecord->type = TypeInteger;
                    break;
                case Double:
                    varRecord->type = TypeDouble;
                    break;
                default:
                    return SEMELSE_ERR;
            }
            return PROG_OK;
        }
    } else if (actualToken.tokenType == Double || actualToken.tokenType == Integer
            || actualToken.tokenType == String || actualToken.tokenType == DocumentString
            || actualToken.tokenType == LeftBracket) {
        PRINT_DEBUG("\tEXPRESSION\n");

        expr = true;
        // Posielame aktuálny token
        if ((errorCode = expression(in, &indentationStack,
                &actualToken, NULL, 1, &ret_type, inFunc)) != 0) return errorCode;

        switch (ret_type) {
            case String:
            case Identifier: varRecord->type = TypeString;
                break;
            case Integer:
                varRecord->type = TypeInteger;
                break;
            case Double:
                varRecord->type = TypeDouble;
                break;
            default:
                return SEMELSE_ERR;
        }

        return PROG_OK;
    } else if (actualToken.tokenType == Keyword) {
        switch (actualToken.tokenAttribute.intValue) {
            case keywordInputf:
                PRINT_DEBUG("\tINPUTF\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                if (cg_input(TypeDouble) == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordInputs:
                PRINT_DEBUG("\tINPUTS\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                if (cg_input(TypeString) == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordInputi:
                PRINT_DEBUG("\tINPUTI\n");
                GET_AND_CHECK_TOKEN(LeftBracket);
                GET_AND_CHECK_TOKEN(RightBracket);
                if (cg_input(TypeInteger) == false) return INTERNAL_ERR;
                return PROG_OK;

            case keywordLen:
                PRINT_DEBUG("\tLEN\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == String || actualToken.tokenType == DocumentString) {
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }
                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie len
                if (cg_fun_call("function_len") == false) return INTERNAL_ERR;

                varRecord->type = TypeInteger;
                return PROG_OK;

            case keywordSubstr:
                PRINT_DEBUG("\tSUBSTR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeString) {
                        printf("%i\n", paramRecord->type);
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    cg_fun_param_assign(1, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(1, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    cg_fun_param_assign(2, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(2, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie substr
                if (cg_fun_call("function_substr") == false) return INTERNAL_ERR;

                varRecord->type = TypeString;
                return PROG_OK;

            case keywordOrd:
                PRINT_DEBUG("\tORD\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == String) {
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeString) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(Comma);

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    cg_fun_param_assign(1, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(1, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie ord
                if (cg_fun_call("function_ord") == false) return INTERNAL_ERR;

                varRecord->type = TypeInteger;
                return PROG_OK;

            case keywordChr:
                PRINT_DEBUG("\tCHR\n");

                GET_AND_CHECK_TOKEN(LeftBracket);

                ADD_INST("CREATEFRAME");

                GET_TOKEN;
                if (actualToken.tokenType == Integer) {
                    if (actualToken.tokenAttribute.intValue > 255)
                        return SEMRUN_ERR;
                    cg_fun_param_assign(0, actualToken, inFunc);
                } else if (actualToken.tokenType == Identifier) {
                    hTabItem_t* paramRecord = isInLocalOrGlobalhTab(actualToken.tokenAttribute.word);
                    if (paramRecord->type != TypeInteger) {
                        return SEMRUN_ERR;
                    } else {
                        cg_fun_param_assign(0, actualToken, inFunc);
                    }
                } else {
                    return SEMRUN_ERR;
                }

                GET_AND_CHECK_TOKEN(RightBracket);

                // Volanie vstavanej funkcie chr
                cg_fun_call("function_len");

                varRecord->type = TypeString;
                return PROG_OK;

            default:
                return SYNTAX_ERR;
        }
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
                if (cg_print_id(var, false) == false) return INTERNAL_ERR;
                // Nachádza sa v lokálnej hashT
            } else if ((var = TSearch(LocalTable, actualToken.tokenAttribute.word)) != NULL) {
                if (cg_print_id(var, true) == false) return INTERNAL_ERR;
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

// Vrati item z hashovacej tabulky, Item z local table ma prednost
hTabItem_t* isInLocalOrGlobalhTab(dynamicString_t name) {
    hTabItem_t* varRecord = NULL;
    if ((varRecord = (TSearch(LocalTable,name))) != NULL && inFunc) {
        return varRecord;
    } else if ((varRecord = (TSearch(GlobalTable,name))) != NULL) {
        return varRecord;
    } else {
        return NULL;
    }
}
