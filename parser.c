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
        if (controlRecord->value.intValue != funcRecord.value.intValue){
            return SEMPARAM_ERR;
        } else {
            inFunc = false;
            return PROG_OK;
        }
    }

    GET_AND_CHECK_TOKEN(Colon);                                         // def foo(...):
    GET_AND_CHECK_TOKEN(Indent);                                        // def foo(...):

    GET_TOKEN;
    if ((errorCode = commandList()) != PROG_OK) return errorCode;       // __command_list

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
    if (actualToken.tokenType == RightBracket) {
        return PROG_OK;
    } else if (actualToken.tokenType == Identifier) {
        funcRecord->value.intValue++;

        // Definovanie parametrov funkcie v lokálnom rámci jedinečním číselným identifikátorom
        if (cg_fun_param_declare(actualToken.tokenAttribute.word.text) == false)  return INTERNAL_ERR;

        GET_TOKEN;

        // Koniec parametrov
        if (actualToken.tokenType == RightBracket) {        // def foo(a)
            return PROG_OK;
        // Nasleduje daľší parameter
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
    PRINT_DEBUG("Command\n");
    // WHILE
    // Vzor: while (výraz): INDENT
    //          sekvencia prikazov <- EOL po kazdom prikaze
    //       DEDENT
    if (actualToken.tokenType == Keyword &&
        actualToken.tokenAttribute.intValue == keywordWhile) {
        PRINT_DEBUG("\tWhile\n");

        // Začiatok while cyklu
        if (cg_while_start(uni_a, uni_b) == false)  return INTERNAL_ERR;

        // Podmienka ďaľšej iterácie cyklu
        if ((errorCode = expression(in,&indentationStack, NULL)) != PROG_OK) return errorCode;

        if (actualToken.tokenType != Colon)         return SYNTAX_ERR;
        GET_AND_CHECK_TOKEN(Indent);

        // Zoznam príkazov v tele cyklu
        if ((errorCode = commandList()) != PROG_OK) return errorCode;

        // Koniec while cyklu
        if (cg_while_end(uni_a, uni_b) == false)    return INTERNAL_ERR;

        uni_a++;     uni_b++;

        return (errorCode = commandListContOrEnd());
    // IF & ELSE
    // Vzor:  if (výraz): INDENT
    //          sekvencia príkazov <- EOL po kazdom prikaze
    //        DEDENT else: INDENT
    //          sekvencia prikazov <- EOL po kazdom prikaze
    //        DEDENT
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordIf) {
        PRINT_DEBUG("\tIf & Else\n");

        // Začiatok vetvenia
        cg_if_start(uni_a, uni_b);

        // Podmienka vetvenia
        if ((errorCode = expression(in,&indentationStack, NULL)) != PROG_OK) return errorCode;

        if (actualToken.tokenType != Colon)         return SYNTAX_ERR;
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
        if (cg_if_end(uni_a, uni_b) == false)       return INTERNAL_ERR;
        uni_a++;     uni_b++;

        return (errorCode = commandListContOrEnd());
    // PRINT
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPrint) {
        PRINT_DEBUG("\tPrint\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        PRINT_DEBUG("\t\tTerms\n");
        // Výpis termov
        if ((errorCode = term()) != PROG_OK) return errorCode;

        return (errorCode = commandListContOrEnd());
    // RETURN
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordReturn) {
        PRINT_DEBUG("\tReturn\n");

        // Príraz návratu sa môže nachádzať len v tele funkcie
        if (!inFunc) return SYNTAX_ERR;

        GET_TOKEN; // TODO možno sa nema volať
        expr = true;
        if ((errorCode = expression(in,&indentationStack, &actualToken)) != 0) return errorCode;

        // TODO RETURN TMP -> tmp vysledok
        // Návrat z tela funkcie
        if (cg_fun_return() == false) return INTERNAL_ERR;

        return (errorCode = commandListContOrEnd());
    // PASS
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordPass) {
        PRINT_DEBUG("\tPass\n");
        // A: TODO PASS
        // Q: Čo sa tú ma vypisovať?
        // A: Asi nič.
        // Q: A sme si istý?
        // A: Nie.
        return (errorCode = commandListContOrEnd());
    // ID
    } else if (actualToken.tokenType == Identifier) {
        PRINT_DEBUG("\tID\n");

        hTabItem_t varRecord; // zaznam premennej
        varRecord.key = actualToken.tokenAttribute.word;
        varRecord.defined = TRUE;
        varRecord.next = NULL;

        GET_TOKEN;

        if (actualToken.tokenType == Assign) {
            // Deklarácia novej premennej
            if (cg_var_declare(varRecord.key.text,inFunc) == false) return INTERNAL_ERR;

            // Aby bola deklarovaná, musí jej byť priradená hodnota
            if ((errorCode = (assign(&varRecord))) != PROG_OK) return errorCode;

            // TODO ak nie je premenná celkovo definovana v tele programu tak je lokálna?
            // TODO v takom prípade ak inBody -> Global, inak Local a Local free vždy ked skončí Command list
            // Uloženie premennej do HashTable
            if (inFunc) {
                TInsert(LocalTable, varRecord);
            } else {
                TInsert(GlobalTable, varRecord);
            }

            return (errorCode = commandListContOrEnd());
        } else if (actualToken.tokenType == LeftBracket) {
            // Ide o funkciu, ak je definovaná, presvedčíme sa že sedí počet parametrov, inak ju pridáme do hTab
            param(&varRecord);
            hTabItem_t* funcRec = NULL;


            if ((funcRec = (TSearch(GlobalTable,varRecord.key))) != NULL) {
                if (funcRec->value.intValue != varRecord.value.intValue)
                    return SEMPARAM_ERR;
            } else {
                varRecord.type = TypeFunction;
                varRecord.defined = FALSE;
                TInsert(GlobalTable,varRecord);
            }

            // Skáčeme do tela funkcie
            if (cg_fun_call(funcRec->key.text) == false) return INTERNAL_ERR;

            return (errorCode = commandListContOrEnd());
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

    // Výpis dokumentačného reťazca
    if (actualToken.tokenType == DocumentString) {
        cg_print_literal(actualToken.tokenAttribute.word.text, TypeString);
    // Výpis hodnoty identifikátora
    } else if (actualToken.tokenType == Identifier) {
        hTabItem_t *var;
        // Nachádza sa v globálnej hashT
        if ((var = TSearch(GlobalTable,actualToken.tokenAttribute.word)) != NULL) {
            if (cg_print_id(var,true) == false)     return INTERNAL_ERR;
        // Nachádza sa v lokálnej hashT
        } else if ((var = TSearch(LocalTable,actualToken.tokenAttribute.word)) != NULL) {
            if (cg_print_id(var,false) == false)     return INTERNAL_ERR;
        // ID nebol definovaný -> ERROR
        } else {
            return SEMPROG_ERR; // nedefinovana premenna TODO možno má vypýsať None
        }
    // Výpis reťazca
    } else if (actualToken.tokenType == String) {
        if (cg_print_literal(actualToken.tokenAttribute.word.text, TypeString) == false) return INTERNAL_ERR;
    // Výpis celého čísla prevedeného na text
    } else if (actualToken.tokenType == Integer) {
        char buffer[100];
        sprintf(buffer,"%d",actualToken.tokenAttribute.intValue);
        if (cg_print_literal(buffer, TypeInteger) == false)             return INTERNAL_ERR;
    // Výpis desatinného čísla prevedeného na text
    } else if (actualToken.tokenType == Double) {
        char buffer[100];
        sprintf(buffer,"%a",actualToken.tokenAttribute.doubleValue);
        if (cg_print_literal(buffer, TypeDouble) == false)              return INTERNAL_ERR;
    // Výpis neznámej hodnoty
    } else if (actualToken.tokenType == Keyword && actualToken.tokenAttribute.intValue == keywordNone) {
        if (cg_print_literal("None", TypeNone) == false)                return INTERNAL_ERR;
    } else {
        return SYNTAX_ERR;
    }

    GET_TOKEN;

    // Končí výpis alebo nasledujú dalšie termy?
    if (actualToken.tokenType == RightBracket) {
        // Koniec výpisu -> EOL
        if (cg_print_literal("\n", TypeString) == false) return INTERNAL_ERR;
        return PROG_OK;
    } else if (actualToken.tokenType == Comma) {
        // Ďaľší term -> medzera
        if (cg_print_literal(" ", TypeString) == false)  return INTERNAL_ERR;
        return (errorCode = term());
    } else {
        return SYNTAX_ERR;
    }
}

int assign(hTabItem_t* varRecord) {
    PRINT_DEBUG("Assignment\n");

    GET_TOKEN;

    // ID
    if (actualToken.tokenType == Identifier) { //abc = abc...
        PRINT_DEBUG("\tID\n");


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
                if (controlRecord->value.intValue != funcRecord.value.intValue)
                    return SEMPARAM_ERR;
            } else {
                // Uložíme funkciu do HashTable a budeme neskor zistovat jej definiciu
                TInsert(GlobalTable, funcRecord);
            }

            // Skáčeme do tela funkcie
            if (cg_fun_call(funcRecord.key.text) == false) return INTERNAL_ERR;

            // Priradzujeme návratovú hodnotu funkcie
            // TODO použiť cg_fun_retval_assign() ?, takto sa neokontroluje malloc
            if (inFunc) { ADD_CODE("MOVE LF@"); } else { ADD_CODE("MOVE GF@"); }
            ADD_CODE(varRecord->key.text); ADD_CODE(" TF@navratova_hodnota");

            return PROG_OK;
        } else {
            // Volanie precedentnej analyzi
            // Riesi sa vyraz, musime odovzdat dva tokeny
            // controlToken a actualToken
            expr = true;
            if ((errorCode = expression(in,&indentationStack, NULL)) != 0) return  errorCode;

            // TODO MOVE var TMP -> tmp vysledok
            if (inFunc) { ADD_CODE("MOVE LF@"); } else { ADD_CODE("MOVE GF@"); }
            ADD_CODE(varRecord->key.text); ADD_CODE(" TF@navratova_hodnota");

            return PROG_OK;
        }
    // EXPRESSION()
    } else if (actualToken.tokenType == Double || actualToken.tokenType == Integer) {
        PRINT_DEBUG("\tExpression\n");

        expr = true;
        if ((errorCode = expression(in,&indentationStack, &actualToken)) != 0) return  errorCode;

        // Priradenie výsledku výrazu
        if (inFunc) { ADD_CODE("MOVE LF@"); } else { ADD_CODE("MOVE GF@"); }
        ADD_CODE(varRecord->key.text); ADD_CODE(" TF@navratova_hodnota");

        return PROG_OK;
    // INPUTF() DONE
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputf) {
        PRINT_DEBUG("\tInputf\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        varRecord->type = TypeDouble;
        if (cg_input(*varRecord,inFunc) == false) return INTERNAL_ERR;

        return PROG_OK;
    // INPUTS() DONE
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputs) {
        PRINT_DEBUG("\tInputs\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        varRecord->type = TypeString;
        if (cg_input(*varRecord,inFunc) == false) return INTERNAL_ERR;

        return PROG_OK;
    // INPUTI() DONE
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordInputi) {
        PRINT_DEBUG("\tInputi\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(RightBracket);

        varRecord->type = TypeInteger;
        if (cg_input(*varRecord,inFunc) == false) return INTERNAL_ERR;

        return PROG_OK;
    // LEN(s)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordLen) {
        PRINT_DEBUG("\tLen\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        // TODO MOVE var STRLEN(s)
        ADD_CODE("CREATEFRAME");
        ADD_CODE("DEFVAR TF@%1");
        ADD_CODE("MOVE TF@%1 string@"); ADD_CODE(dynamicStringGetText(actualToken.tokenAttribute.word));

        GET_AND_CHECK_TOKEN(RightBracket);

        // Volanie vstavanej funkcie len
        if (cg_fun_call("FUNCTION_LEN") == false) return INTERNAL_ERR;

        // Priradenie dĺžky zadaného retazca
        if (cg_frame_assign_retval(*varRecord, inFunc) == false) return INTERNAL_ERR;

        varRecord->type = TypeInteger;
        return PROG_OK;
    // SUBSTR(s,i,n)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordSubstr) {
        PRINT_DEBUG("\tSubstr\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        // TODO MOVE str String
        ADD_CODE("CREATEFRAME");
        ADD_CODE("DEFVAR TF@%1");
        ADD_CODE("MOVE TF@%1 string@"); ADD_CODE(dynamicStringGetText(actualToken.tokenAttribute.word));
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        // TODO MOVE int Int
        ADD_CODE("DEFVAR TF@%2");
        ADD_CODE("MOVE TF@%2 int@");
        ADD_CODE_INT(actualToken.tokenAttribute.intValue);
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        // TODO MOVE int Int
        ADD_CODE("DEFVAR TF@%3");
        ADD_CODE("MOVE TF@%3 int@"); ADD_CODE_INT(actualToken.tokenAttribute.intValue);
        GET_AND_CHECK_TOKEN(RightBracket);

        // Volanie vstavanej funkcie substr
        if (cg_fun_call("FUNCTION_SUBSTR") == false) return INTERNAL_ERR;

        // Priradenie podreťazca
        if (cg_frame_assign_retval(*varRecord, inFunc) == false) return INTERNAL_ERR;

        varRecord->type = TypeString;
        return PROG_OK;
    // ORD(s,i)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordOrd) {
        PRINT_DEBUG("\tOrd\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(String);
        // TODO MOVE str String
        ADD_CODE("CREATEFRAME");
        ADD_CODE("DEFVAR TF@%1");
        ADD_CODE("MOVE TF@%1 string@"); ADD_CODE(dynamicStringGetText(actualToken.tokenAttribute.word));
        GET_AND_CHECK_TOKEN(Comma);
        GET_AND_CHECK_TOKEN(Integer);
        // TODO MOVE int Int
        ADD_CODE("DEFVAR TF@%2");
        ADD_CODE("MOVE TF@%2 int@");
        ADD_CODE_INT(actualToken.tokenAttribute.intValue);
        GET_AND_CHECK_TOKEN(RightBracket);

        // Volanie vstavanej funkcie ord
        if (cg_fun_call("FUNCTION_ORD") == false) return INTERNAL_ERR;

        // Priradenie ordinálnej hodnoty
        if (cg_frame_assign_retval(*varRecord, inFunc) == false) return INTERNAL_ERR;

        varRecord->type = TypeInteger;
        return PROG_OK;
    // CHR(i)
    } else if (actualToken.tokenType == Keyword &&
               actualToken.tokenAttribute.intValue == keywordChr) {
        PRINT_DEBUG("\tChr\n");

        GET_AND_CHECK_TOKEN(LeftBracket);
        GET_AND_CHECK_TOKEN(Integer);

        // TODO MOVE str 'Int'
        ADD_CODE("CREATEFRAME");
        ADD_CODE("DEFVAR TF@%1");
        ADD_CODE("MOVE TF@%1 int@");
        ADD_CODE_INT(actualToken.tokenAttribute.intValue);
        GET_AND_CHECK_TOKEN(RightBracket);

        // Volanie vstavanej funkcie chr
        cg_fun_call("FUNCTION_CHR");

        // Priradenie znaku
        if (cg_frame_assign_retval(*varRecord, inFunc) == false) return INTERNAL_ERR;

        varRecord->type = TypeString;
        return PROG_OK;
    // ERROR
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
