/**
 * Implementation of imperative language IFJ2019 compiler
 * @file code-gen.c
 * @author Pavol Dubovec (xdubov02), Juraj Lazur (xlazur00)
 * @brief Code generator implementation
 */

#include "code-gen.h"
#include "dynamic-string.h"
#include "symtable.h"

#include <stdio.h>

// Global code variable
dynamicString_t code;
// Global Symbol table
hashTable* global_table;

void set_code_output(dynamicString_t* output, hashTable* global) {
    code = *output;
    global_table = global;
}

int code_write_out(int errorCode) {
    // Ak bol program korektný, je výsledný kód IFJcode19 vypísaný na stdout
    if (errorCode == PROG_OK) {

        // Zápis na stdout
        fprintf(stdout, "%s", code.text);
        // Uvoľnenie DynamicString
        dynamicStringFree(&code);

        return PROG_OK;
    } else {
        // V prípade, že vstupný program nebol korektný, nie je vypísané nič
        dynamicStringFree(&code);
        return errorCode;
    }
}

bool cg_code_header()
{
    ADD_INST(".IFJcode19");
    ADD_INST("# Program start");

    ADD_INST("DEFVAR GF@$$expr_result");
    ADD_INST("DEFVAR GF@$$op_1");
    ADD_INST("DEFVAR GF@$$op_2");
    ADD_INST("DEFVAR GF@$$typ_op_1");
    ADD_INST("DEFVAR GF@$$concat");
    ADD_INST("DEFVAR GF@$$typ_op_2");
    ADD_INST("DEFVAR GF@$$expr_result_type");

    ADD_INST("JUMP $$main");

    return true;
}

bool cg_define_b_i_functions()
{
    ADD_INST(FUNCTION_LEN);
    ADD_INST(FUNCTION_SUBSTR);
    ADD_INST(FUNCTION_ORD);
    ADD_INST(FUNCTION_CHR);

    return true;
}

bool cg_main_scope() {
    ADD_INST("\n# Zaciatok main");
    ADD_INST("LABEL $$main");
    ADD_INST("CREATEFRAME");
    ADD_INST("PUSHFRAME");

    return true;
}

bool cg_code_footer()
{
    ADD_INST("\n# Koniec main");
    ADD_INST("POPFRAME");
    ADD_INST("CLEARS");

    return true;
}

bool cg_fun_start(char *id_funkcie)
{
    ADD_CODE("\n# Začiatok funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_CODE("LABEL $"); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_INST("PUSHFRAME");

    return true;
}

bool cg_fun_end(char *id_funkcie)
{
    ADD_CODE("\n# Koniec funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_INST("POPFRAME");
    ADD_INST("RETURN");
    return true;
}

bool cg_fun_param_declare(char* param, int num)
{
    ADD_CODE("DEFVAR LF@"); ADD_CODE(param); ADD_CODE("\n");
    ADD_CODE("MOVE LF@"); ADD_CODE(param); ADD_CODE(" LF@%"); ADD_CODE_INT(num); ADD_CODE("\n");

    return true;
}

bool cg_fun_param_assign(unsigned int id, token_t actualToken, bool local) {
    ADD_CODE("DEFVAR TF@%"); ADD_CODE_INT(id); ADD_CODE("\n");
    ADD_CODE("MOVE TF@%"); ADD_CODE_INT(id); ADD_CODE(" ");

    switch (actualToken.tokenType) {
        case Identifier:
            if (cg_LForGF(local) == false) return false;
            ADD_CODE(actualToken.tokenAttribute.word.text); ADD_CODE("\n");
            return true;
        case Integer:
            ADD_CODE("int@"); ADD_CODE_INT(actualToken.tokenAttribute.intValue); ADD_CODE("\n");
            return true;
        case Double:
            ADD_CODE("float@"); ADD_CODE_DOUBLE(actualToken.tokenAttribute.doubleValue); ADD_CODE("\n");
            return true;
        case String:
        case DocumentString:
        case Keyword: // None
            ADD_CODE("string@");

            char* string = actualToken.tokenAttribute.word.text;
            for (unsigned long i = 0; i < strlen(string); i++) {
                if (string[i] <= 32 || string[i] == 35 || string[i] == 92) {
                    char buffer[4];
                    sprintf(buffer,"%d",string[i]);
                    ADD_CODE("\\0"); ADD_CODE(buffer);
                } else {
                    char c[2];
                    c[1] = '\0';
                    c[0] = string[i];
                    ADD_CODE(c);
                }
            }
            ADD_CODE("\n");
            return true;
        default:
            return false;
    }
}

bool cg_fun_call(char *id_funkcie)
{
    ADD_CODE("CALL $"); ADD_CODE(id_funkcie); ADD_CODE("\n");

    return true;
}

bool cg_fun_return()
{
    ADD_INST("POPFRAME");
    ADD_INST("RETURN");
    return true;
}

bool cg_assign_expr_result(char* variable, bool local) {
    ADD_CODE("MOVE ");
    if (cg_LForGF(local) == false) return false;
    ADD_CODE(variable); ADD_CODE(" GF@$$expr_result");
    ADD_CODE("\n");

    return true;
}

bool cg_var_declare(char* varName, dynamicString_t *in_string, bool local)
{
    dynamicStringAddString(in_string, "DEFVAR ");
    if (!local){
        dynamicStringAddString(in_string, "GF@");
        dynamicStringAddString(in_string, varName);
        dynamicStringAddString(in_string, "\n");
    } else {
        dynamicStringAddString(in_string, "LF@");
        dynamicStringAddString(in_string, varName);
        dynamicStringAddString(in_string, "\n");
    }
    return true;
}

bool cg_label(char* part_1, char* part_2, unsigned int id)
{
    ADD_CODE("LABEL $"); ADD_CODE(part_1); ADD_CODE(part_2); ADD_CODE_INT(id); ADD_CODE("\n");
    return true;
}

bool cg_while_start(unsigned int id)
{
    cg_type_of_symb("$$expr_result_type", "$$expr_result");
    cg_jump("JUMPIFEQ", "$WHILE_bool", id, "", "GF@$$expr_result_type", "string@bool");
    cg_jump("JUMPIFEQ", "$WHILE_int", id, "", "GF@$$expr_result_type", "string@int");

    cg_exit(4);

    cg_label("$WHILE_int", "", id);
    cg_jump("JUMPIFEQ", "$WHILE_int0", id, "", "GF@$$expr_result", "int@0");
    ADD_CODE("MOVE GF@$$expr_result bool@true\n");
    cg_jump("JUMP", "$WHILE_bool", id, "", "", "");
    cg_label("$WHILE_int0", "", id);
    ADD_CODE("MOVE GF@$$expr_result bool@false\n");

    cg_label("$WHILE_bool", "", id);

    ADD_CODE("JUMPIFNEQ $"); ADD_CODE("WHILE");ADD_CODE("A"); ADD_CODE_INT(id); ADD_CODE(" GF@$$expr_result bool@true"); ADD_CODE("\n");

    return true;
}


bool cg_while_end(unsigned int id)
{
    ADD_CODE("JUMP $"); ADD_CODE("WHILE"); ADD_CODE_INT(id); ADD_CODE("\n");

    return true;
}

bool cg_if_start(unsigned int id)
{
    cg_type_of_symb("$$expr_result_type", "$$expr_result");
    cg_jump("JUMPIFEQ", "$IF_bool", id, "", "GF@$$expr_result_type", "string@bool");
    cg_jump("JUMPIFEQ", "$IF_int", id, "", "GF@$$expr_result_type", "string@int");

    cg_exit(4);

    cg_label("$IF_int", "", id);
    cg_jump("JUMPIFEQ", "$IF_int0", id, "", "GF@$$expr_result", "int@0");
    ADD_CODE("MOVE GF@$$expr_result bool@false\n");
    cg_jump("JUMP", "$IF_bool", id, "", "", "");
    cg_label("$IF_int0", "", id);
    ADD_CODE("MOVE GF@$$expr_result bool@true\n");

    cg_label("$IF_bool", "", id);
    ADD_CODE("JUMPIFEQ $"); ADD_CODE("IF"); ADD_CODE_INT(id); ADD_CODE(" GF@$$expr_result bool@false\n");
    return true;
}


bool cg_if_else_part(unsigned int id)
{
    ADD_CODE("JUMP $"); ADD_CODE("IF"); ADD_CODE("A"); ADD_CODE_INT(id); ADD_CODE("\n");
    ADD_INST("# Else part");
    if (!cg_label("IF", "", id)) return false;
    return true;
}

bool cg_if_end(unsigned int id)
{
    ADD_INST("\n# End If");
    if (!cg_label("IF", "A", id)) return false;
    return true;
}

// WRITE <symb>
bool cg_print_literal(char* value, varType_t type) {
    ADD_INST("\n# Function PRINT LITERAL");

    // Výpis celého čísla, desatinného čísla alebo reťazca?
    switch (type) {
        case TypeInteger:
            ADD_CODE("WRITE int@"); ADD_CODE(value);
            return true;
        case TypeDouble:
            ADD_CODE("WRITE float@"); ADD_CODE(value);
            return true;
        case TypeNone:
            ADD_INST("WRITE string@None");
            return true;
        case TypeString:
            ADD_CODE("WRITE string@");
            char buffer[4];
            char c[2];

            // Špeciálne znaky treba previezť na escape sekvencie v tvare \xyz
            for (unsigned long i = 0; i < strlen(value); i++) {
                if (value[i] <= 32 || value[i] == 35 || value[i] == 92) {
                    sprintf(buffer,"%d",value[i]);
                    ADD_CODE("\\0"); ADD_CODE(buffer);
                    buffer[0] = '\0';
                    buffer[1] = '\0';
                    buffer[2] = '\0';
                    buffer[3] = '\0';
                } else {
                    c[1] = '\0';
                    c[0] = value[i];
                    ADD_CODE(c);
                    c[0] = '\0';
                }
            }
            ADD_CODE("\n");
            return true;
        default:
            return false;
    }
}

bool cg_print_id(hTabItem_t* varRecord, bool local) {
    ADD_INST("\n# Function PRINT ID");

    ADD_CODE("WRITE ");
    if (cg_LForGF(local) == false) return false;
    ADD_CODE(varRecord->key.text); ADD_CODE("\n");
    return true;
}

// Výpis typu podla hodnoty z enum-u
bool cg_type(varType_t type) {
    switch (type) {
        case TypeInteger:
            ADD_CODE("int");        return true;
        case TypeString:
            ADD_CODE("string");     return true;
        case TypeDouble:
            ADD_CODE("float");      return true;
        case TypeBool:
            ADD_CODE("bool");       return true;
        default:
            return false;
    }
}

// READ <var> <type>
bool cg_input(varType_t type) {
    ADD_INST("\n# Reading variable");
    ADD_CODE("READ GF@$$expr_result ");
    if (cg_type(type) == false) return false; ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty symb na vrchol dátoveho zasobniku
bool cg_stack_push_id(char* symb) {
    ADD_CODE("PUSHS ");
    // Nachádza sa v globálnej hashTable
    if (TSearch_char(global_table, symb)) {
        ADD_CODE("GF@");
    } else {
        ADD_CODE("LF@");
    }
    ADD_CODE(symb);
    ADD_CODE("\n");
    return true;
}

// Uloženie hodnoty symb na vrchol dátoveho zasobniku
bool cg_stack_push_gl(char* symb) {
    ADD_CODE("PUSHS ");
    ADD_CODE("GF@");
    ADD_CODE(symb);
    ADD_CODE("\n");
    return true;
}

// Uloženie hodnoty symb na vrchol dátoveho zasobniku
bool cg_stack_push_literal(varType_t type, char* val) {
    ADD_CODE("PUSHS ");

    if (cg_type(type) == false)     return false;

    ADD_CODE("@");
    char buffer[4];
    char c[2];

    for (unsigned long i = 0; i < strlen(val); i++) {
        if (val[i] <= 32 || val[i] == 35 || val[i] == 92) {
            sprintf(buffer,"%d",val[i]);
            ADD_CODE("\\0"); ADD_CODE(buffer);
            buffer[0] = '\0';
            buffer[1] = '\0';
            buffer[2] = '\0';
            buffer[3] = '\0';
        } else {
            c[1] = '\0';
            c[0] = val[i];
            ADD_CODE(c);
            c[0] = '\0';
        }
    }
    ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty val na vrchol dátoveho zasobniku
bool cg_stack_push_int(int val) {
    ADD_CODE("PUSHS int@");
    ADD_CODE_INT(val); ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty val na vrchol dátoveho zasobniku
bool cg_stack_push_double(double val) {
    ADD_CODE("PUSHS ");
    ADD_CODE_DOUBLE(val); ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty z vrcholu dátového zásobníka do symb
bool cg_stack_pop_id(char* var, bool local) {
    ADD_CODE("POPS ");
    if (cg_LForGF(local) == false) return false;
    ADD_CODE(var); ADD_CODE("\n");

    return true;
}

// Clear IFJcode19 stack
bool cg_clear_stack() {
    ADD_INST("CLEARS");

    return true;
}

// Matematické operácie nad operandmi na vrchole dátového zásobníka
bool cg_math_operation_stack(parserState_t operation) {
    switch (operation) {
        case Plus:
            ADD_INST("ADDS");       return true;
        case Minus:
            ADD_INST("SUBS");       return true;
        case Multiply:
            ADD_INST("MULS");       return true;
        case DivideWRest:
            ADD_INST("DIVS");       return true;
        case DivideWORest:
            ADD_INST("IDIVS");      return true;
        default:
            return false;
    }
}

// Relačné operácie na vrchole dátového zásobníka
bool cg_rel_operation_stack(parserState_t operation) {
    switch (operation) {
        case NotEqual:
            ADD_INST("EQS");
            ADD_INST("NOTS");            return true;
        case Smaller:
            ADD_INST("LTS");            return true;
        case SmallerOrEqual:
            ADD_INST("LTS");
            cg_stack_push_gl("$$op_2");
            cg_stack_push_gl("$$op_1");
            ADD_INST("EQS");
            ADD_INST("ORS");            return true;
        case Bigger:
            ADD_INST("GTS");            return true;
        case BiggerOrEqual:
            ADD_INST("GTS");
            cg_stack_push_gl("$$op_2");
            cg_stack_push_gl("$$op_1");
            ADD_INST("EQS");
            ADD_INST("ORS");            return true;
        case Equals:
            ADD_INST("EQS");            return true;
        default:
            return false;
    }
}

// Dynamické zisťovanie typu premmennej
bool cg_type_of_symb(char* var, char* symb){
    ADD_CODE("TYPE GF@"); ADD_CODE(var); ADD_CODE(" GF@"); ADD_CODE(symb); ADD_CODE("\n");
    return true;
}

// Konverzia int na double nad zásobníkom
bool cg_stack_int2float(){
    ADD_INST("INT2FLOATS");
    return true;
}

// Exit z programu pocas interpretacie
bool cg_exit(int errorNum){
    ADD_CODE("EXIT int@"); ADD_CODE_INT(errorNum); ADD_CODE("\n");
    return true;
}

// JUMP inštrukcia, všetky jej varianty
bool cg_jump(char* jump_type, char* flag_1_part, unsigned int flag_number, char* flag_2_part, char* op_1, char* op_2){
    ADD_CODE(jump_type); ADD_CODE(" $"); ADD_CODE(flag_1_part); ADD_CODE_INT(flag_number); ADD_CODE(flag_2_part); ADD_CODE(" ");
    ADD_CODE(op_1); ADD_CODE(" "); ADD_CODE(op_2); ADD_CODE("\n");
    return true;
}

// Hlavná funkcia pre vykonávanie matematických operácií nad operandmi, zahŕňa ja konkatenáciu stringov
bool cg_operation(unsigned int operation, unsigned int id){
    ADD_CODE("\n");
    cg_jump("JUMPIFNEQ", "$operation_m", id, "", "GF@$$typ_op_1", "string@string");

    cg_stack_pop_id("$$op_2", false);
    cg_stack_pop_id("$$op_1", false);

    ADD_CODE("CONCAT GF@$$expr_result GF@$$op_1 GF@$$op_2\n");

    cg_stack_push_gl("$$expr_result");

    cg_jump("JUMP", "$operation_f", id, "", "", "");

    cg_label("$operation_m","", id);

    cg_math_operation_stack(operation);

    cg_label("$operation_f","", id);
    ADD_CODE("\n");

    return true;
}

bool cg_LForGF(bool local) {
    if (local) { ADD_CODE("LF@"); } else { ADD_CODE("GF@"); }

    return true;
}
