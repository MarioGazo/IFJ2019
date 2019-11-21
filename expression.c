
#include "error.h"
#include "dynamic-symstack.h"
#include <stdlib.h>
#include <stdio.h>

#define RULEHEIGHT 13
#define RULEWIDTH 3


char LL[7][7] = {
// +|- *|/|// (    )    i    EV   $
  {'>', '<', '<', '>', '<', '>', '>'}, //+
  {'>', '>', '<', '>', '<', '>', '>'}, //*
  {'<', '<', '<', '=', '<', '<', ' '}, //(
  {'>', '>', ' ', '>', ' ', '>', '>'}, //)
  {'>', '>', ' ', '>', ' ', '>', '>'}, //i
  {'<', '<', '<', '>', '<', '>', '>'}, //==, >, <, etc
  {'<', '<', '<', ' ', '<', '<', ' '}, //$

};
//Rules to pick from. 99 = E (Nonterminal)
 parserState_t rules[RULEHEIGHT][RULEWIDTH] = {
  {99, Plus, 99},                                           //E -> E+E
  {99, Minus, 99},                                          //E -> E-E
  {99, DivideWRest, 99},                                    //E -> E/E
  {99, DivideWORest, 99},                                   //E -> E//E
  {99, Multiply, 99},                                       //E -> E*E
  {99, NotEqual, 99},                                       //E -> E!=E
  {99, SmallerOrEqual, 99},                                 //E -> E>=E
  {99, Smaller, 99},                                        //E -> E>E
  {99, Bigger, 99},                                         //E -> E<E
  {99, BiggerOrEqual, 99},                                  //E -> E<=E
  {99, Equals, 99},                                         //E -> E==E
  {RightBracket, 99, LeftBracket},                          //E -> (E) (in reverse because of the stack)
  {Identifier, (parserState_t) NULL, (parserState_t) NULL}  //E -> i ()
};

token_t * terminalTop(dynamic_symbol_stack_t * stack, int * deep){
  token_t * token = sym_stackTopItem(stack);
  int i = 0;

  while(token != NULL && token->tokenType == 99){
    i++;
    token = sym_stackTraverse(stack, i);
  }
  *deep = i;
  return token;
}

int LLPos(token_t * token){
  switch (token->tokenType){
    case Plus:
    case Minus:
      return 0;
      break;
    case Multiply:
    case DivideWRest:
    case DivideWORest:
      return 1;
      break;
    case LeftBracket:
      return 2;
      break;
    case RightBracket:
      return 3;
      break;
    case Identifier:
    case Integer:
    case Double:
      return 4;
      break;
    case NotEqual:
    case Smaller:
    case SmallerOrEqual:
    case Bigger:
    case BiggerOrEqual:
    case Equals:
      return 5;
      break;
    case  EOL:
    case  Comma:
    case  EndOfFile:
      return 6;
      break;
    default:

      return -1;

  };
  return -1;
}
token_t * new_token(parserState_t type){
  token_t * token = calloc(1, sizeof(token_t));
  token->tokenType = type;
  return token;
}
token_t * getNewToken(){
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
    default:
      return new_token(EOL);
      break;


  };
}

int expression(token_t * token) {
     dynamic_symbol_stack_t * stack = sym_stackInit();
     token_t * end = calloc(1, sizeof(token_t));
     token_t * bufferT = NULL;
     parserState_t exp[RULEWIDTH];
     int * deep = calloc(1, sizeof(int));


     end->tokenType = EOL;
     sym_stackPush(stack, end); //push $ as the first item of the stack
     if(token==NULL){ //in case the token was not passed as the function argument, get a new one
        token = getNewToken();

     }

     do{

       switch(LL[LLPos(terminalTop(stack, deep))][LLPos(token)]){
          case '=':

            sym_stackPush(stack, token);
            token = getNewToken();
            break;
          case '<':

            sym_stackDeepInsert(stack, new_token(100), *deep);  //insert < behind the first terminal (100 = enum shift)
            sym_stackPush(stack, token);
            sym_stackPrint(stack);//TODO: delete this line

            token = getNewToken();
            break;
          case '>':

            //clear the comparison array
            for(int i = 0; i<RULEWIDTH; i++){
              exp[i] = (parserState_t) NULL;
            }
            int index = 0;
            int stop = 0;
            //fill the array with token types from max RULEWIDTH (probably 3) top items from the stack (until the shift token) and then dispose of them (including the shift one) as they are no longer necessary
            do{
              bufferT = sym_stackPop(stack);
              if(bufferT->tokenType != 100){
                exp[index] = bufferT->tokenType;
                free(bufferT);
                index++;
              }else{
                free(bufferT);
                stop = 1;
              }

            }while(!stop);
            //try to find a match
            int found = -1;
            printf("Match: ");
            for(int i = 0; i<RULEHEIGHT; i++){//go through all the rules and compare their token types
              int match = 0;
              for(int o = 0; o<RULEWIDTH; o++){
                if(rules[i][o] == exp[o]){
                  match ++; //each time a match is found, increase the match value by 1. If after this row match = RULEWIDTH, it means that the rule has been found and that it is this row
                  printf("%d", i);
                }
              }
              if(match == RULEWIDTH){
                found = i;
                break; //A rule has been found so there is no need to continue searching
              }
            }
            printf("\n");
            if(found>-1){
              sym_stackPush(stack, new_token(99)); //All rules are assumed to have a left side E, because they have. (E = 99)
              //TODO: printing out the expressions in target language
            }else{

              printf("%s\n", "SYNTAX ERROR RULE NOT FOUND");
              return SYNTAX_ERR;
            }
            break;
          case ' ': //Empty LL cell, meaning a syntax error
            printf("%s\n", "SYNTAX ERROR");
            return SYNTAX_ERR;
            break;

       }





       sym_stackPrint(stack);

     }while(!(token->tokenType==EOL && sym_stackTopItem(stack)->tokenType==99 && (sym_stackTraverse(stack, 1)->tokenType== EOL || sym_stackTraverse(stack, 1)->tokenType== EndOfFile || sym_stackTraverse(stack, 1)->tokenType== Comma))); //The work is over when there is E$ on the stack and the token is one of the forementioned types



    sym_stackFree(stack);
    free(deep);
    return PROG_OK;
}


int main(){
  expression(NULL);


  //stack test
  return 1;
  dynamic_symbol_stack_t * stack = sym_stackInit();
  token_t * token = calloc(1, sizeof(token_t));
  token->tokenType = EOL;
  sym_stackPush(stack, token);
  token_t * token1 = calloc(1, sizeof(token_t));
  token1->tokenType = 99;
  sym_stackPush(stack, token1);


  sym_stackPrint(stack);

  sym_stackFree(stack);

}
