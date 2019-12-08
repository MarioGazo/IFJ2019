/**
 * Implementation of imperative language IFJ2019 compiler
 * @file expression.c
 * @author Vojtěch Golda (xgolda02), Juraj Lazur (xlazur00)
 * @brief Expression parser implementation
 */

#include "error.h"
#include "dynamic-symstack.h"
#include <stdlib.h>
#include <stdio.h>
#include "code-gen.h"

#define DEBUG 0 // TODO set to 0
#define RULEHEIGHT 18
#define RULEWIDTH 3

#define PRINT_DEBUG(text) \
    if (DEBUG) \
        printf(text)

FILE* in;
dynamic_stack_t* iStack;

//variables to deal with the getToken function
token_t* microStack = NULL;
static unsigned int uni_2_a = 0;
int zero_division = 0;
int inFun = 0;


int errN = PROG_OK;
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
    {DocumentString, (parserState_t) NULL, (parserState_t) NULL},       // E -> s 14
    {Integer, (parserState_t) NULL, (parserState_t) NULL},      // E -> integer 15
    {Double, (parserState_t) NULL, (parserState_t) NULL},        // E -> double 16
    {Keyword, (parserState_t) NULL, (parserState_t) NULL}       // E -> None 17
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
    if (token->tokenType == Keyword){
        if (!strcmp(token->tokenAttribute.word.text, "None")){
            return 4;
        } else {
            return -1;
        }
    }
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
        case String:
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
        case Identifier:
        case DocumentString:
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
    if (token == NULL)
        return NULL;

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
    if (token == NULL)
        return NULL;

    *token = getToken(in,iStack);

    if (DEBUG) {
        printToken(iStack, *token);
    }

    if (token->tokenType == Error)       errN = LEX_ERR;
    if (token->tokenType == ErrorMalloc) errN = INTERNAL_ERR;
    if (token->tokenType == ErrorIndent) errN = LEX_ERR;
    if(errN != PROG_OK) return NULL;
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

int expSwitch( dynamic_symbol_stack_t* stack, token_t** t, const int* depth, char symbol, int *ret_value_type){
    token_t* bufferT = NULL;
    token_t* token  = *t;
    token_t* exp[RULEWIDTH];
    switch(symbol){
        case '=':
            sym_stackPush(stack, token);
            *t = getNewToken();
            if (errN != PROG_OK)
                return errN;
            break;
        case '<':
            //insert < behind the first terminal (100 = enum shift)
            sym_stackDeepInsert(stack, new_token(100), *depth);
            sym_stackPush(stack, token);
            if (DEBUG)
                sym_stackPrint(stack);

            *t = getNewToken();
            if (errN != PROG_OK)
                return errN;
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
                if (E == NULL)
                    return INTERNAL_ERR;

                if(found == 12 ||found == 13 ||found == 14 ||found == 15 ||found == 16 ||found == 17){
                  E->tokenAttribute.intValue = exp[0]->tokenType;

                  if (((E->tokenAttribute.intValue == Integer) && (exp[0]->tokenAttribute.intValue == 0)) ||
                      ((E->tokenAttribute.intValue == Double) && (exp[0]->tokenAttribute.doubleValue == 0.0))){
                        E->tokenAttribute.intValue = 0;
                  }

                  cg_stack_p(exp[0]);
                  free(exp[0]); //the rest of exp is empty
                }else if(found != 11){
                    if ((exp[0]->tokenType == 99) && (exp[2]->tokenType == 99)){

                        if ((exp[0]->tokenAttribute.intValue == DocumentString) || (exp[0]->tokenAttribute.intValue == String)
                        || (exp[2]->tokenAttribute.intValue == String) || (exp[2]->tokenAttribute.intValue == DocumentString)){
                            if ((exp[1]->tokenType != Plus) && (exp[1]->tokenType != Equals) && (exp[1]->tokenType != NotEqual) &&
                               (exp[1]->tokenType != SmallerOrEqual) && (exp[1]->tokenType != Smaller) && (exp[1]->tokenType != Bigger) &&
                               (exp[1]->tokenType != BiggerOrEqual)){
                                return SEMRUN_ERR;
                            }
                        }

                        if (((exp[0]->tokenAttribute.intValue == DocumentString) || (exp[0]->tokenAttribute.intValue == String))
                            && ((exp[2]->tokenAttribute.intValue != String) && (exp[2]->tokenAttribute.intValue != DocumentString)
                            && (exp[2]->tokenAttribute.intValue != Identifier) && (exp[2]->tokenAttribute.intValue != Keyword))){
                                return SEMRUN_ERR;
                        }

                        if (((exp[2]->tokenAttribute.intValue == DocumentString) || (exp[2]->tokenAttribute.intValue == String))
                            && ((exp[0]->tokenAttribute.intValue != String) && (exp[0]->tokenAttribute.intValue != DocumentString)
                            && (exp[0]->tokenAttribute.intValue != Identifier) && (exp[0]->tokenAttribute.intValue != Keyword))){
                                return SEMRUN_ERR;
                        }

                        if (exp[1]->tokenType == DivideWRest){
                            if (exp[0]->tokenAttribute.intValue == Start){
                                return DIVZERO_ERR;
                            } else {
                                exp[0]->tokenAttribute.intValue = Integer;
                            }

                            if ((exp[0]->tokenAttribute.intValue == Integer) || (exp[0]->tokenAttribute.intValue == Double) || (exp[0]->tokenAttribute.intValue == Identifier)){
                            } else {
                                return  SEMRUN_ERR;
                            }

                            if ((exp[2]->tokenAttribute.intValue == Integer) || (exp[2]->tokenAttribute.intValue == Double) || (exp[2]->tokenAttribute.intValue == Identifier)){
                            } else {
                                return  SEMRUN_ERR;
                            }
                        } else if (exp[1]->tokenType == DivideWORest){
                            if (exp[0]->tokenAttribute.intValue == Start){
                                return DIVZERO_ERR;
                            } else if (exp[0]->tokenAttribute.intValue == Integer) {
                                exp[0]->tokenAttribute.intValue = Integer;
                            }

                            if ((exp[0]->tokenAttribute.intValue == Integer) || (exp[0]->tokenAttribute.intValue == Identifier)){
                            } else {
                                return  SEMRUN_ERR;
                            }

                            if ((exp[2]->tokenAttribute.intValue == Integer) || (exp[2]->tokenAttribute.intValue == Identifier)){
                            } else {
                                return  SEMRUN_ERR;
                            }
                        }

                        E->tokenAttribute.intValue = cg_count(exp[1]->tokenType, exp[0]->tokenAttribute.intValue, exp[2]->tokenAttribute.intValue, ret_value_type);
                    } else if (exp[0]->tokenType == 99){
                        E->tokenAttribute.intValue = cg_count(exp[1]->tokenType, exp[0]->tokenAttribute.intValue, exp[2]->tokenType, ret_value_type);
                    } else if (exp[2]->tokenType == 99){
                        E->tokenAttribute.intValue = cg_count(exp[1]->tokenType, exp[0]->tokenType, exp[2]->tokenAttribute.intValue, ret_value_type);
                    } else {
                        E->tokenAttribute.intValue = cg_count(exp[1]->tokenType, exp[0]->tokenType, exp[2]->tokenType, ret_value_type);
                    }
                  free(exp[0]);
                  free(exp[1]);
                  free(exp[2]);
                }else{
                  //(E) Rule
                  free(exp[0]);
                  free(exp[1]);
                  free(exp[2]);
                }
                sym_stackPush(stack, E);
                //TODO: printing out the expressions in target language
            } else {
                PRINT_DEBUG("SYNTAX ERROR RULE NOT FOUND\n");
                return SYNTAX_ERR;
            }

            break;
        default: //Empty LL cell, meaning a syntax error
            if (DEBUG)
                printf("%d\n", token->tokenType);

            PRINT_DEBUG("SYNTAX ERROR\n");
            return SYNTAX_ERR;
    }
    return PROG_OK;
}

bool cg_stack_p(token_t* token){
    if (token->tokenType == DocumentString || token->tokenType == String) {
        cg_stack_push_literal(TypeString, token->tokenAttribute.word.text);
    } else if (token->tokenType == Integer){
        cg_stack_push_int(token->tokenAttribute.intValue);
    } else if (token->tokenType == Identifier){
        cg_stack_push_id(token->tokenAttribute.word.text);
    } else if (token->tokenType == Double){
        cg_stack_push_double(token->tokenAttribute.doubleValue);
    }

    return true;
}

int cg_count(parserState_t operatio, unsigned int type_op_1, unsigned int type_op_2, int *ret_value_type){

    cg_stack_pop_id("op_1", false);
    cg_stack_pop_id("op_2", false);

    cg_stack_push_gl("op_2");
    cg_stack_push_gl("op_1");

    cg_type_of_symb("typ_op_1", "op_1");
    cg_type_of_symb("typ_op_2", "op_2");

    cg_jump("JUMPIFEQ", "data_control", uni_2_a, "final", "GF@typ_op_1", "GF@typ_op_2");
    cg_jump("JUMPIFNEQ", "data_control", uni_2_a, "not_string", "GF@typ_op_1", "string@string");
    cg_jump("JUMPIFNEQ", "data_control", uni_2_a, "not_string", "GF@typ_op_2", "string@string");

    cg_exit(4);

    cg_flag_gen("data_control", uni_2_a, "not_string");
    cg_jump("JUMPIFEQ", "data_control", uni_2_a, "int", "GF@typ_op_1", "string@int");

    cg_stack_pop_id("op_1", false);
    cg_stack_int2float();
    cg_stack_push_gl("op_1");

    cg_jump("JUMP", "data_control", uni_2_a, "final", "", "");

    cg_flag_gen("data_control", uni_2_a, "int");
    cg_stack_int2float();

    cg_flag_gen("data_control", uni_2_a, "final");


    // Vykoname operaciu
    // Operácia môže byť matematická, alebo relačná
    // V prípade relačnej, výsledok odvzdávame späť, buď ako bool v prípade podmienky,
    // alebo ako výsledok výrazu v prípade priradenia
    if ((operatio == Plus) || (operatio == Minus) || (operatio == Multiply) ||
        (operatio == DivideWRest) || (operatio == DivideWORest)){
        // Len operacia nad zasobnikom
        cg_math_operation_stack(operatio);
    } else if (operatio == Assign) {
        // Priradenie a odovzdanie vysledku cez navratova_hodnota
        cg_stack_pop_id("op_1", false);
        cg_move("op_1", "typ_op_1");
        cg_clear_stack();

    } else if ((operatio == NotEqual) || (operatio == Smaller) || (operatio == SmallerOrEqual)
    || (operatio == Bigger) || (operatio == BiggerOrEqual) || (operatio == Equals)){
        // Relacna operacia nad zasobnikom
        cg_rel_operation_stack(operatio);
    }

    uni_2_a = uni_2_a + 1;

    if (type_op_1 == type_op_2){
        *ret_value_type = type_op_1;
        return type_op_1;
    } else if ((type_op_1 == Double) || (type_op_2 == Double)){
        *ret_value_type = Double;
        return Double;
    } else {
        *ret_value_type = Integer;
        return Integer;
    }

}

int expression(FILE* lIn, dynamic_stack_t* lIStack, token_t* t, token_t* controlToken, int amountOfPassedTokens, int* ret_value_type, int inF){
    //promote the passed variables to the global scope
    in = lIn;
    iStack = lIStack;

    dynamic_symbol_stack_t * stack = sym_stackInit();
    if (stack == NULL)
        return INTERNAL_ERR;

    token_t * end = calloc(1, sizeof(token_t));
    if (end == NULL)
        return INTERNAL_ERR;

    int d;
    int* depth = &d;
    int retCode = 0;
    inFun = inF;
    // Which LLpos function to pick from. 0 means undecided. 1 is concatenation mode, -1 is expression mode.
    // Any further attempt to change it while it has a non-zero value should result in an error.
    int mode = 0;
    token_t* token;
    end->tokenType = EOL;
    sym_stackPush(stack, end); // Push $ as the first item of the stack

    switch(amountOfPassedTokens){
      case 0: // No tokens were passed so just get a new one
        token = getNewToken();
        if (errN != PROG_OK)
            return errN;
        *ret_value_type = token->tokenType;
        break;
      case 1:
        token = calloc(1, sizeof(token_t));  // Only one token was passed so use that one instead of a new one
        if (token == NULL)
            return INTERNAL_ERR;
        *token = *t;
        *ret_value_type = token->tokenType;
        break;
      case 2: // Two were passed - im assuming that controlToken comes before t, (TODO: maybe switch them)
        token = calloc(1, sizeof(token_t)); //both of these should end up on the stack at some point
        if (token == NULL)
            return  INTERNAL_ERR;
        microStack = calloc(1, sizeof(token_t));
        if (microStack == NULL)
            return INTERNAL_ERR;
        *token = *controlToken;
        *ret_value_type = token->tokenType;


        //simulate 'returning' a token to the scanner using the microStack and an if statement in the getNewToken() function
        *microStack = *t;
        break;
      default:
        return INTERNAL_ERR;

    }

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
            retCode = expSwitch(stack, &token, depth, LL[LLPos(terminalTop(stack, depth))][LLPos(token)], ret_value_type);
        } else {
            retCode = expSwitch(stack, &token, depth, LL[LLSPos(terminalTop(stack, depth))][LLSPos(token)], ret_value_type);
        }

        if (retCode!=PROG_OK) {
            sym_stackFree(stack);
            free(token);
            return retCode;
        }

        if (DEBUG)
            sym_stackPrint(stack);
    } while(!(sym_stackTraverse(stack, 1)->tokenType ==EOL && sym_stackTopItem(stack)->tokenType == 99 &&
               (token->tokenType == EOL || token->tokenType == EndOfFile || token->tokenType == Colon ||
                token->tokenType == Dedent || token->tokenType == Indent)));
    //The work is over when there is E$ on the stack and the token is one of the forementioned types

    cg_stack_pop_id("expr_result", false);

     sym_stackFree(stack);
     if(t!=NULL){
       *t = *token; //return the last token
     }
     free(token);
     return PROG_OK;
}
