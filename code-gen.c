/**
 * Implementation of imperative language IFJ2019 compiler
 * @file code-gen.c
 * @author Pavol Dubovec (xdubov02)
 * @brief Code generator implementation
 */

#include "code-gen.h"
#include "dynamic-string.h"

#include <stdio.h>

// Global code variable
dynamicString_t code;

void set_code_output(dynamicString_t* output) {
    code = *output;
}

int code_write_out(int errorCode) {
    FILE * output;
    // Ak je program korektný, je výsledný kód vypísaný do súboru
    if (errorCode == PROG_OK) {
        output = fopen("prg.out","w+");

        // Zlyhanie pri tvorbe súboru na výpis
        if (output == NULL) {
            return INTERNAL_ERR;
        }

        fprintf(output, "%s", code.text);
        dynamicStringFree(&code);
        fclose(output);

        return PROG_OK;
    } else {
        dynamicStringFree(&code);
        return errorCode;
    }
}

bool cg_code_header()
{
    ADD_INST(".IFJcode19");
    ADD_INST("# Program start");

    ADD_INST("DEFVAR GF@expr_result");

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
    ADD_INST("CREATEFRAME");

    return true;
}

bool cg_fun_end(char *id_funkcie)
{
    ADD_CODE("\n# Koniec funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_CODE("LABEL $"); ADD_CODE(id_funkcie); ADD_CODE("%return\n");
    ADD_INST("POPFRAME");
    ADD_INST("RETURN");
    return true;
}

bool cg_fun_param_declare(char *id_parametra)
{
    ADD_CODE("DEFVAR LF@"); ADD_CODE(id_parametra); ADD_CODE("\n");

    return true;
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
    if (local) { ADD_CODE("LF@"); } else { ADD_CODE("GF@"); }
    ADD_CODE(variable);     ADD_CODE("\n");

    return true;
}

bool cg_var_declare(char* varName, bool inFunc)
{
    ADD_CODE("DEFVAR ");
    if (inFunc) {ADD_CODE("LF@");} else {ADD_CODE("GF@");}
    ADD_CODE(varName); ADD_CODE("\n");
    return true;
}

bool cg_label(char* funcName, unsigned int uni_a,unsigned int uni_b)
{
    ADD_CODE("LABEL $"); ADD_CODE(funcName); ADD_CODE("%"); ADD_CODE_INT(uni_a);
    ADD_CODE("%"); ADD_CODE_INT(uni_b); ADD_CODE("\n");
    return true;
}

bool cg_while_start(unsigned int uni_a, unsigned int uni_b)
{
    ADD_INST("\n# Beginning of while");
    if (!cg_label("WHILE", uni_a, uni_b)) return false;

    ADD_CODE("JUMPIFEQ $"); ADD_CODE("WHILE"); ADD_CODE("%"); ADD_CODE_INT(uni_b);
    ADD_CODE("%"); ADD_CODE_INT(uni_a); ADD_CODE(" GF@%exp_result bool@false"); ADD_CODE("\n");
    return true;
}


bool cg_while_end(unsigned int uni_a, unsigned int uni_b)
{
    ADD_CODE("JUMP $"); ADD_CODE("WHILE"); ADD_CODE("%"); ADD_CODE_INT(uni_a);
    ADD_CODE("%"); ADD_CODE_INT(uni_b); ADD_CODE("\n");
    ADD_INST("# End of while");

    if (!cg_label("WHILE", uni_b, uni_a)) return false;
    return true;
}


bool cg_if_start(unsigned int uni_a, unsigned int uni_b)
{
    ADD_INST("\n# Begin of If Then");
    ADD_CODE("JUMPIFEQ $"); ADD_CODE("IF"); ADD_CODE("%"); ADD_CODE_INT(uni_a);
    ADD_CODE("%"); ADD_CODE_INT(uni_b); ADD_CODE(" GF@%exp_result bool@false\n");
    return true;
}


bool cg_if_else_part(unsigned int uni_a, unsigned int uni_b)
{
    ADD_CODE("JUMP $"); ADD_CODE("IF"); ADD_CODE("%"); ADD_CODE_INT(uni_b);
    ADD_CODE("%"); ADD_CODE_INT(uni_a); ADD_CODE("\n");
    ADD_INST("# Else part");
    if (!cg_label("IF", uni_a, uni_b)) return false;
    return true;
}

bool cg_if_end(unsigned int uni_a, unsigned int uni_b)
{
    ADD_INST("# End If");
    if (!cg_label("IF", uni_b, uni_a)) return false;
    return true;
}

// WRITE <symb>
bool cg_print_literal(char* value, varType_t typ) {
    ADD_INST("\n# Function PRINT LITERAL");

    // Výpis celého čísla, desatinného čísla alebo reťazca?
    switch (typ) {
        case TypeInteger:
            ADD_CODE("WRITE int@"); ADD_CODE(value);
            return true;
        case TypeDouble:
            ADD_CODE("WRITE string@"); ADD_CODE(value);
            return true;
        case TypeNone:
            ADD_INST("WRITE string@None"); // TODO možno nil@nil
            return true;
        case TypeString:
            ADD_CODE("WRITE string@");
            // Špeciálne znaky treba previezť na escape sekvencie v tvare \xyz
            for (unsigned long i = 0; i < strlen(value); i++) {
                if (value[i] <= 32 || value[i] == 35 || value[i] == 92) {
                    char buffer[4];
                    sprintf(buffer,"%d",value[i]);
                    ADD_CODE("\\0"); ADD_CODE(buffer);
                } else {
                    char c[2];
                    c[1] = '\0';
                    c[0] = value[i];
                    ADD_CODE(c);
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
    if (local) { ADD_CODE("GF@"); } else { ADD_CODE("LF%"); }
    ADD_CODE(varRecord->key.text); ADD_CODE("\n");
    return true;
}

// Výpis typu podla hodnoty z enum-u
bool cg_type(varType_t type) {
    switch (type) {
        case TypeInteger:
            ADD_CODE("int@");        return true;
        case TypeString:
            ADD_CODE("string@");     return true;
        case TypeDouble:
            ADD_CODE("float@");      return true;
        case TypeBool:
            ADD_CODE("bool@");       return true;
        default:
            return false;
    }
}

// READ <var> <type>
bool cg_input(hTabItem_t variable,bool local) {
    // Je premenná uložená v lokálnom alebo globálnom rámci?
    ADD_INST("# Reading variable");
    if (local) { ADD_CODE("READ LF@"); } else { ADD_CODE("READ GF@"); }

    // Meno premennej
    ADD_CODE(variable.key.text); ADD_CODE(" ");

    // Typ premennej
    if (cg_type(variable.type) == false) return false;
    ADD_CODE("\n");

    return true;
}

// Operácia nad operandmi a premenná do ktorej je výsledok priradený
bool cg_math_operation(parserState_t operation, char* var, char* op1, char* op2) {
    switch (operation) {
        case Plus:
            ADD_CODE("ADD ");  ADD_CODE(" ");
                       ADD_CODE(var); ADD_CODE(" ");
                       ADD_CODE(op1); ADD_CODE(" ");
                       ADD_CODE(op2); ADD_CODE("\n");
            break;
        case Minus:
            ADD_CODE("SUB ");  ADD_CODE(" ");
                       ADD_CODE(var); ADD_CODE(" ");
                       ADD_CODE(op1); ADD_CODE(" ");
                       ADD_CODE(op2); ADD_CODE("\n");
            break;
        case Multiply:
            ADD_CODE("MUL ");  ADD_CODE(" ");
                       ADD_CODE(var); ADD_CODE(" ");
                       ADD_CODE(op1); ADD_CODE(" ");
                       ADD_CODE(op2); ADD_CODE("\n");
            break;
        case DivideWRest:
            ADD_CODE("DIV ");  ADD_CODE(" ");
                       ADD_CODE(var); ADD_CODE(" ");
                       ADD_CODE(op1); ADD_CODE(" ");
                       ADD_CODE(op2); ADD_CODE("\n");
            break;
        case DivideWORest:
            ADD_CODE("IDIV ");  ADD_CODE(" ");
                        ADD_CODE(var); ADD_CODE(" ");
                        ADD_CODE(op1); ADD_CODE(" ");
                        ADD_CODE(op2); ADD_CODE("\n");
            break;
        default:
            return false;
    }

    return true;
}

// Uloženie hodnoty symb na vrchol dátoveho zasobniku
bool cg_stack_push_id(char* symb, bool local) {
    ADD_CODE("PUSHS "); if (local) {ADD_CODE("LF@");} else {ADD_CODE("GF@");} ADD_CODE(symb);

    return true;
}

// Uloženie hodnoty symb na vrchol dátoveho zasobniku
bool cg_stack_push_literal(varType_t type, char* val) {
    ADD_CODE("PUSHS ");

    if (cg_type(type) == false)     return false;

    ADD_CODE(val); ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty val na vrchol dátoveho zasobniku
bool cg_stack_push_int(unsigned int val) {
    ADD_CODE("PUSHS ");
    ADD_CODE_INT(val); ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty val na vrchol dátoveho zasobniku
bool cg_stack_push_double(double val) {
    ADD_CODE("PUSHS ");
    ADD_CODE_DOUBLE(val); ADD_CODE("\n");

    return true;
}

// Uloženie hodnoty symb na vrchol dátoveho zasobniku
bool cg_stack_pop_id(char* var, bool local) {
    ADD_CODE("POPS ");
    if (local) { ADD_CODE("LF@"); } else { ADD_CODE("GF@"); }
    ADD_CODE(var); ADD_CODE("\n");

    return true;
}

// Clear stack
bool cg_clear_stack() {
    ADD_INST("CLEARS");

    return true;
}

// Operácia nad operandmi na vrchole dátového zásobníka
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

// Konkatenácia operandov a priradenie premennej
bool cg_cat_literal(char* var, char* op1, char* op2) {
    ADD_CODE("CONCAT"); ADD_CODE(var); ADD_CODE(op1); ADD_CODE(op2); ADD_CODE("\n");

    return true;
}

// Konkatenácia operandov a priradenie premennej
bool cg_cat_id(char* var, bool local1, char* op1, bool local2, char* op2, bool local3) {
    ADD_CODE("CONCAT");
    if (local1) { ADD_CODE("LF@"); } else { ADD_CODE("GF@"); } ADD_CODE(var);
    if (local2) { ADD_CODE("LF@"); } else { ADD_CODE("GF@"); } ADD_CODE(op1);
    if (local3) { ADD_CODE("LF@"); } else { ADD_CODE("GF@"); } ADD_CODE(op2);
    ADD_CODE("\n");

    return true;
}

// Dynamicke zistenie typu premmennej
bool cg_type_of_symb(char* var, char* symb){
    ADD_CODE("TYPE LF@"); ADD_CODE(var); ADD_CODE(" LF@"); ADD_CODE(symb); ADD_CODE("\n");
    return true;
}

// Vytvorenie navestia
bool cg_flag_gen(char* a_part, unsigned int number, char* b_part){
    ADD_CODE("LABEL $"); ADD_CODE(a_part); ADD_CODE("$"); ADD_CODE_INT(number); ADD_CODE("$"); ADD_CODE(b_part); ADD_CODE("\n");
    return true;
}

// Konverzia int na double
bool cg_stack_int2float(){
    ADD_CODE("INT2FLOATS"); ADD_CODE("\n");
    return true;
}

// Exit z programu pocas interpretacie
bool cg_exit(int errorNum){
    ADD_CODE("EXIT int@"); ADD_CODE_INT(errorNum); ADD_CODE("\n");
    return true;
}

// JUMP - vsetky varianty
bool cg_jump(char* jump_type, char* flag_1_part, unsigned int flag_number, char* flag_2_part, char* op_1, char* op_2){
    ADD_CODE(jump_type); ADD_CODE(" $"); ADD_CODE(flag_1_part); ADD_CODE("$"); ADD_CODE_INT(flag_number); ADD_CODE("$"); ADD_CODE(flag_2_part); ADD_CODE(" ");
    ADD_CODE(op_1); ADD_CODE(" "); ADD_CODE(op_2); ADD_CODE("\n");
    return true;
}
