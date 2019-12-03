/**
 * Implementation of imperative language IFJ2019 compiler
 * @file expression.c
 * @author Vojtěch Golda (xgolda02)
 * @brief Expression parser implementation
 */

#include "error.h"
#include "dynamic-symstack.h"
#include <stdlib.h>
#include <stdio.h>
#include "code-gen.h"

#define DEBUG 1 // TODO set to 0
#define RULEHEIGHT 16
#define RULEWIDTH 3

#define PRINT_DEBUG(text) \
    if (DEBUG) \
        printf(text)

FILE* in;
dynamic_stack_t* iStack;
//variables to deal with the getToken function

token_t* microStack = NULL;
static unsigned int uni_2_a = 0;



int errN = 0;
char LL[7][7] = {
  // +|- *|/|// (    )    i    EV   $
    {'>', '<', '<', '>', '<', '>', '>'}, // +
    {'>', '>', '<', '>', '<', '>', '>'}, // *
    {'<', '<', '<', '=', '<', '<', ' '}, // (
    {'>', '>', ' ', '>', ' ', '>', '>'}, // )
    {'>', '>', ' ', '>', ' ', '>', '>'}, // i
    {'<', '<', '<', '>', '<', '>', '>'}, // ==, >, <, etc
    {'<', '<', '<', ' ', '<', '<', ' '}, // $
};

//Rules to pick from. 99 = E (Nonterminal)
parserState_t rules[RULEHEIGHT][RULEWIDTH] = {
    {Nonterminal, Plus, Nonterminal},                           // E -> E+E 1
    {Nonterminal, Minus, Nonterminal},                          // E -> E-E 2
    {Nonterminal, DivideWRest, Nonterminal},                    // E -> E/E 3
    {Nonterminal, DivideWORest, Nonterminal},                   // E -> E//E 4
    {Nonterminal, Multiply, Nonterminal},                       // E -> E*E 5
    {Nonterminal, NotEqual, Nonterminal},                       // E -> E!=E 6
    {Nonterminal, SmallerOrEqual, Nonterminal},                 // E -> E>=E 7
    {Nonterminal, Smaller, Nonterminal},                        // E -> E>E 8
    {Nonterminal, Bigger, Nonterminal},                         // E -> E<E 9
    {Nonterminal, BiggerOrEqual, Nonterminal},                  // E -> E<=E 10
    {Nonterminal, Equals, Nonterminal},                         // E -> E==E 11
    {RightBracket, Nonterminal, LeftBracket},                   // E -> (E) (in reverse because of the stack) 12
    {Identifier, (parserState_t) NULL, (parserState_t) NULL},   // E -> i () 13
    {String, (parserState_t) NULL, (parserState_t) NULL},       // E -> s 14
    {Integer, (parserState_t) NULL, (parserState_t) NULL},      // E -> integer 15
    {Double, (parserState_t) NULL, (parserState_t) NULL}        // E -> double 16
};


token_t* terminalTop(dynamic_symbol_stack_t * stack, int * depth) {
    token_t* token = sym_stackTopItem(stack);
    int i = 0;

    while(token != NULL && token->tokenType == 99) {
        i++;
        token = sym_stackTraverse(stack, i);
    }

    *depth = i;
    return token;
}

int LLPos(token_t* token) {
    switch (token->tokenType) {
        case Plus:
        case Minus:
            return 0;
        case Multiply:
        case DivideWRest:
        case DivideWORest:
            return 1;
        case LeftBracket:
            return 2;
        case RightBracket:
            return 3;
        case Identifier:
        case Integer:
        case Double: 
            return 4;
        case NotEqual:
        case Smaller:
        case SmallerOrEqual:
        case Bigger:
        case BiggerOrEqual:
        case Equals:
            return 5;
        case  EOL:
        case  Colon:
        case  EndOfFile:
        case  Dedent:
        case  Indent:
            return 6;
        default:
            return -1;
    }
}

int LLSPos(token_t* token) {
    switch (token->tokenType) {
        case Plus:
            return 0;
        case LeftBracket:
            return 2;
        case RightBracket:
            return 3;
        case String:
            return 4;
        case  EOL:
        case  Colon:
        case  EndOfFile:
        case  Dedent:
        case  Indent:
            return 6;
        default:
            return -1;
    }
}

token_t* new_token(parserState_t type){
    token_t * token = calloc(1, sizeof(token_t));
    token->tokenType = type;
    return token;
}

token_t* getNewToken() {

    if(microStack != NULL){ // If there is a token on the microStack, return that one instead of a new one and clear it
        token_t* token = microStack;
        microStack = NULL;
        return token;
    }
    token_t* token = calloc(1, sizeof(token_t));

    *token = getToken(in,iStack);

    if (DEBUG) {
        printToken(iStack, *token);
    }

    if (token->tokenType == Error)       errN = LEX_ERR;
    if (token->tokenType == ErrorMalloc) errN = INTERNAL_ERR;
    if (token->tokenType == ErrorIndent) errN = SYNTAX_ERR;
    if(errN != 0) return NULL;
    return token;
  /*
  char  s;
  scanf(" %c", &s);

  switch (s){
    case '+':
      return new_token(Plus);
      break;
    case '-':
      return new_token(Minus);
      break;
    case '*':
      return new_token(Multiply);
      break;
    case '(':
      return new_token(LeftBracket);
      break;
    case ')':
      return new_token(RightBracket);
      break;
    case 'i':
      return new_token(Identifier);
      break;
    case '=':
      return new_token(Equals);
      break;
    case 's':
      return new_token(String);
      break;
    default:
      return new_token(EOL);
      break;
  };
  */
}

int expSwitch( dynamic_symbol_stack_t* stack, token_t** t, const int* depth, char symbol){
    token_t* bufferT = NULL;
    token_t* token  = *t;
    token_t* exp[RULEWIDTH];
    switch(symbol){
        case '=':
            sym_stackPush(stack, token);
            *t = getNewToken();
            break;
        case '<':
            //insert < behind the first terminal (100 = enum shift)
            sym_stackDeepInsert(stack, new_token(100), *depth);
            sym_stackPush(stack, token);
            sym_stackPrint(stack);//TODO: delete this line

            *t = getNewToken();
            break;
        case '>':
            //clear the comparison array
            for(int i = 0; i<RULEWIDTH; i++){
                exp[i] = (token_t*) NULL;
            }
            int index = 0;
            int stop = 0;
            // fill the array with token types from max RULEWIDTH (probably 3) top items from the stack (until the
            // shift token) and then discard the shift token
            do {
                bufferT = sym_stackPop(stack);
                if(bufferT->tokenType != 100) {
                    exp[index] = bufferT;
                    index++;
                } else {
                    free(bufferT);
                    stop = 1;
                }

            } while(!stop);

            //try to find a match
            int found = -1;
            PRINT_DEBUG("Match: ");
            for(int i = 0; i<RULEHEIGHT; i++) { // go through all the rules and compare their token types
                int match = 0;
                for(int o = 0; o<RULEWIDTH; o++) {
                    //sadly they are not the same kind of NULL
                    if((exp[o] != NULL &&  rules[i][o] == exp[o]->tokenType) ||
                       (exp[o] == NULL &&  rules[i][o] == (parserState_t) NULL) ) {
                        // each time a match is found, increase the match value by 1. If after this row
                        // match = RULEWIDTH, it means that the rule has been found and that it is this row
                        match++;
                        PRINT_DEBUG("| ");
                    }
                }

                //A rule has been found so there is no need to continue searching
                if(match == RULEWIDTH){
                    found = i;
                    break;
                }
            }
            PRINT_DEBUG("\n");

            // To prevent attempting to reach the tokenAttribute of a NULL token. Hopefully if it finds the token to be
            // NULL it wont carry on with the if statement
            if ((found == 2 || found == 3) &&
                 exp[2] != NULL &&
                (exp[2]->tokenAttribute.intValue == 0 &&
                 exp[2]->tokenAttribute.doubleValue == 0.0 )) { //Zero div check
            // Rules involving DIVISION, If they try to divide by a string its an error
            // regardless so its not like its a wrong way to test for zero

            PRINT_DEBUG("DIVISION BY ZERO (or a string) ERROR\n");
            return DIVZERO_ERR;
            }

            if(found>-1) {
                //All rules are assumed to have a left side E, because they do. (E = 99)
                token_t * E = new_token(99);


                if(found == 13 ||found == 14 ||found == 15 ||found == 12){
                  E->tokenType = exp[0]->tokenType;
                  cg_stack_p(exp[0]);
                  //TODO: push |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

                  free(exp[0]); //the rest of exp is empty
                }else if(found != 11){
                  E->tokenType = cg_count(exp[1]->tokenType, exp[0]->tokenType, exp[2]->tokenType);
                  free(exp[0]);
                  free(exp[1]);
                  free(exp[2]);
                }else{
                  //(E) Rule
                  free(exp[0]);
                  free(exp[1]);
                  free(exp[2]);
                }
                sym_stackPush(stack, new_token(99));
                //TODO: printing out the expressions in target language
            } else {
                PRINT_DEBUG("SYNTAX ERROR RULE NOT FOUND\n");
                return SYNTAX_ERR;
            }

            break;
        case ' ': //Empty LL cell, meaning a syntax error
            if (DEBUG)
                printf("%d\n", token->tokenType);

            PRINT_DEBUG("SYNTAX ERROR\n");
            return SYNTAX_ERR;
    }
    return PROG_OK;
}

bool cg_stack_p(token_t* token){
    if (token->tokenType == Identifier || token->tokenType == DocumentString || token->tokenType == String) {
        cg_stack_push_literal(TypeString, token->tokenAttribute.word.text);
    } else if (token->tokenType == Integer){
        cg_stack_push_int(token->tokenAttribute.intValue);
    } else if (token->tokenType == Double){
        cg_stack_push_double(token->tokenAttribute.doubleValue);
    }

    return true;
}

int cg_count(parserState_t operatio, int type_op_1, int type_op_2){



    // Rozdelime si pripady podla typov operandov
    if (type_op_1 == Identifier){
        if (type_op_2 == Identifier){
            // dve premenne, musime zistit ich datove typy
            cg_stack_pop_id("op_1", false);
            cg_stack_pop_id("op_2", false);

            cg_stack_push_id("op_2", false);
            cg_stack_push_id("op_1", false);

            cg_type_of_symb("typ_op_1", "op_1");
            cg_type_of_symb("typ_op_2", "op_2");

            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_1", "LF@typ_op_2");
            cg_jump("JUMPIFNEQ", "data_control", uni_2_a, "not_string", "LF@typ_op_1", "string@string");
            cg_jump("JUMPIFNEQ", "data_control", uni_2_a, "not_string", "LF@typ_op_2", "string@string");

            cg_exit(4);

            cg_flag_gen("data_control", uni_2_a, "not_string");
            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "convert_int", "LF@typ_op_1", "string@int");

            cg_stack_pop_id("op_1", false);
            cg_stack_int2float();
            cg_stack_push_id("op_1", false);

            cg_jump("JUMP", "data_control", uni_2_a, "final", "", "");

            cg_flag_gen("data_control", uni_2_a, "convert_int");
            cg_stack_int2float();

            cg_flag_gen("data_control", uni_2_a, "final");
        } else {
            // minimalne type_op_2 nie je premenna, musime teda zistit, ci je mozne spustenie operacie
            cg_stack_pop_id("op_1", false);
            cg_type_of_symb("typ_op_1", "op_1");
            cg_stack_push_id("op_1", false);

            if (type_op_2 == Integer){
                cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_1", "string@int");
                cg_jump("JUMPIFEQ", "data_control", uni_2_a, "convert", "LF@typ_op_1", "string@float");

                cg_exit(4);

                cg_flag_gen("data_control", uni_2_a, "convert");

                cg_stack_pop_id("op_1", false);
                cg_stack_int2float();
                cg_stack_push_id("op_1", false);

                cg_flag_gen("data_control", uni_2_a, "final");
            }

            if (type_op_2 == Double){
                cg_jump("JUMPIFEQ", "data_control", uni_2_a, "convert", "LF@typ_op_1", "string@int");
                cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_1", "string@float");

                cg_exit(4);

                cg_flag_gen("data_control", uni_2_a, "convert");
                cg_stack_int2float();

                cg_flag_gen("data_control", uni_2_a, "final");
            }

            if (type_op_2 == String){
                cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_1", "string@string");

                cg_exit(4);

                cg_flag_gen("data_control", uni_2_a, "final");
            }

        }
    } else {
        // minimalne type_op_1 nie je premenna, musime teda zistit, ci je mozne spustenie operacie
        cg_stack_pop_id("op_1", false);
        cg_stack_pop_id("op_2", false);

        cg_stack_push_id("op_2", false);
        cg_stack_push_id("op_1", false);

        cg_type_of_symb("typ_op_2", "op_2");

        if (type_op_1 == Integer){
            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_2", "string@int");
            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "convert", "LF@typ_op_2", "string@float");

            cg_exit(4);

            cg_flag_gen("data_control", uni_2_a, "convert");
            cg_stack_int2float();

            cg_flag_gen("data_control", uni_2_a, "final");
        }

        if (type_op_1 == Double){
            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "convert", "LF@typ_op_2", "string@int");
            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_2", "string@float");

            cg_exit(4);

            cg_flag_gen("data_control", uni_2_a, "convert");

            cg_stack_pop_id("op_1", false);
            cg_stack_int2float();
            cg_stack_push_id("op_1", false);

            cg_flag_gen("data_control", uni_2_a, "final");
        }

        if (type_op_1 == String){
            cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "LF@typ_op_2", "string@string");

            cg_exit(4);

            cg_flag_gen("data_control", uni_2_a, "final");
        }
    }


    // Vykoname operaciu
    cg_math_operation_stack(operatio);


    uni_2_a = uni_2_a + 1;

    if (type_op_1 == type_op_2){
        return type_op_1;
    } else if ((type_op_1 == Identifier) || (type_op_2 == Identifier)){
        return Identifier;
    } else if ((type_op_1 == Double) || (type_op_2 == Double)){
        return Double;
    } else if ((type_op_1 == String) || (type_op_2 == String)) {
        return String;
    } else {
        return Integer;
    }

}

int expression(FILE* lIn, dynamic_stack_t* lIStack, token_t* t, token_t* controlToken, int amountOfPassedTokens){
    //promote the passed variables to the global scope
    in = lIn;
    iStack = lIStack;

    dynamic_symbol_stack_t * stack = sym_stackInit();

    token_t * end = calloc(1, sizeof(token_t));
    int d;
    int* depth = &d;
    int retCode = 0;
    // Which LLpos function to pick from. 0 means undecided. 1 is concatenation mode, -1 is expression mode.
    // Any further attempt to change it while it has a non-zero value should result in an error.
    int mode = 0;
    token_t* token;
    end->tokenType = EOL;
    sym_stackPush(stack, end); // Push $ as the first item of the stack

    switch(amountOfPassedTokens){
      case 0: // No tokens were passed so just get a new one
        token = getNewToken();
        break;
      case 1:
        token = calloc(1, sizeof(token_t));  // Only one token was passed so use that one instead of a new one
        *token = *t;
        break;
      case 2: // Two were passed - im assuming that controlToken comes before t, (TODO: maybe switch them)
        token = calloc(1, sizeof(token_t)); //both of these should end up on the stack at some point
        microStack = calloc(1, sizeof(token_t));
        *token = *controlToken;


        //simulate 'returning' a token to the scanner using the microStack and an if statement in the getNewToken() function
        *microStack = *t;
        break;

    }
    cg_var_declare("op_1", false);
    cg_var_declare("op_2", false);
    cg_var_declare("typ_op_1", false);
    cg_var_declare("typ_op_2", false);

    do {
        if (LLPos(token) == -1 && LLSPos(token) == -1) {
            //The token wasnt recognized by neither function, meaning a syntax error
            if (DEBUG)
                printf("TOKEN UNRECOGNIZED: %d\n", token->tokenType);

            sym_stackFree(stack);
            free(token);
            return SYNTAX_ERR;
        } else if (LLPos(token) > -1 && LLSPos(token) > -1) {
        //The token was recognized by both, in which case i do nothing
        } else if (LLPos(token) > -1 && LLSPos(token) == -1) {
        //The token wasnt recognized by the string function, but was by the other
            if( mode == 0) {
                mode = -1;
            } else if(mode == 1) {
                PRINT_DEBUG("WRONG TOKEN (E MODE)\n");
                sym_stackFree(stack);
                free(token);
                return SYNTAX_ERR;
            }
        } else if (LLPos(token) == -1 && LLSPos(token) > -1) {
        //The token was recognized by the string function, but wasnt by the other
            if(mode == 0) {
                mode = 1;
            } else if(mode == -1) {
                PRINT_DEBUG("WRONG TOKEN (S MODE)\n");
                sym_stackFree(stack);
                free(token);
                return SYNTAX_ERR;
            }
        }

        if (mode == 0 || mode == -1) {
            retCode = expSwitch(stack, &token, depth, LL[LLPos(terminalTop(stack, depth))][LLPos(token)]);
        } else {
            retCode = expSwitch(stack, &token, depth, LL[LLSPos(terminalTop(stack, depth))][LLSPos(token)]);
        }

        if (retCode!=0) {
            sym_stackFree(stack);
            free(token);
            return retCode;
        }

        sym_stackPrint(stack);
    } while(!(sym_stackTraverse(stack, 1)->tokenType ==EOL && sym_stackTopItem(stack)->tokenType == 99 &&
               (token->tokenType == EOL || token->tokenType == EndOfFile || token->tokenType == Colon ||
                token->tokenType == Dedent || token->tokenType == Indent)));
    //The work is over when there is E$ on the stack and the token is one of the forementioned types

     sym_stackFree(stack);
     if(t!=NULL){
       *t = *token; //return the last token
     }
     free(token);
     return PROG_OK;
}
