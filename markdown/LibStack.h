/* Stack ADT Implementation.
    - Kevin Hira
*/

typedef struct {
    int *items, capacity, initialised, size;
} Stack;

int StackInitialise(Stack *stack, int capacity);
int StackFree(Stack *stack);
int StackIsInitialised(Stack *stack);
int StackSize(Stack *stack);
int StackCapacity(Stack *stack);
int StackIsFull(Stack *stack);
int StackIsEmpty(Stack *stack);
int StackPush(Stack *stack, int data);
int StackPull(Stack *stack, int *data);
int StackPeek(Stack *stack, int *peek);
