#include <stdio.h>
#include "scanner.h"

#define DEBUG 1

int main() {
    token_t actualToken;
    actualToken.tokenType = Start;

    // indentation stack
    dynamic_stack_t indentationStack;
    stackInit(&indentationStack);

#if DEBUG
    parserState_t state = Start;
    while (actualToken.tokenType != EndOfFile &&
           actualToken.tokenType != Error &&
           actualToken.tokenType != ErrorMalloc) {
        actualToken = getToken(stdin,&indentationStack);
        state = actualToken.tokenType;
        switch (state) {
            case Plus:
                printf("Plus\n");
                break;
            case Minus:
                printf("Minus\n");
                break;
            case Multiply:
                printf("Multiply\n");
                break;
            case Colon:
                printf("Colon\n");
                break;
            case LeftBracket:
                printf("LeftBracket\n");
                break;
            case RightBracket:
                printf("RightBracket\n");
                break;
            case Comma:
                printf("Comma\n");
                break;
            case DivideWORest:
                printf("DivideWORest\n");
                break;
            case DivideWRest:
                printf("DivideWRest\n");
                break;
            case NotEqual:
                printf("NotEqual\n");
                break;
            case Smaller:
                printf("Smaller\n");
                break;
            case SmallerOrEqual:
                printf("Smaller\n");
                break;
            case Bigger:
                printf("Bigger\n");
                break;
            case BiggerOrEqual:
                printf("BiggerOrEqual\n");
                break;
            case Assign:
                printf("Assign\n");
                break;
            case Equals:
                printf("Equals\n");
                break;
            case Indent:
                printf("Indent: ");
                for (int i = 0; i < indentationStack.top + 1; i++) {
                    printf("%d ",indentationStack.data[i]);
                }
                printf("\n");
                break;
            case Dedent:
                printf("Dedent: ");
                for (int i = 0; i < indentationStack.top + 1; i++) {
                    printf("%d ",indentationStack.data[i]);
                }
                printf("\n");
                break;
            case DocumentStringEnd:
                printf("DocumentStringEnd = ");
                printf("%s\n",actualToken.tokenAttribute.word.text);
                break;
            case StringEnd:
                printf("StringEnd = ");
                printf("%s\n",actualToken.tokenAttribute.word.text);
                break;
            case Identifier:
                printf("Identifier = ");
                printf("%s\n",actualToken.tokenAttribute.word.text);
                break;
            case Keyword:
                printf("Keyword\n");
                break;
            case Integer:
                printf("Integer = ");
                printf("%d\n",actualToken.tokenAttribute.intValue);
                break;
            case Double:
                printf("Double = ");
                printf("%lf\n",actualToken.tokenAttribute.doubleValue);
                break;
            case Error:
                printf("Error\n");
                break;
            case ErrorMalloc:
                printf("ErrorMalloc\n");
                break;
            case EndOfFile:
                printf("EndOfFile\n");
                break;
            default:
                printf("Start\n");
        }
    }
#endif

    stackFree(&indentationStack);
    return 0;
}
