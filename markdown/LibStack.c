#include "LibStack.h"
#include <stdio.h>
#include <stdlib.h>

int StackInitialise(Stack *stack, int capacity)
{
    if (StackIsInitialised(stack) || (stack->items=(int*)calloc(capacity,sizeof(int))) == NULL)
        return 0;
    stack->initialised = 1;
    stack->capacity = capacity;
    stack->size = 0;
    return 1;
}

int StackFree(Stack *stack)
{
    if (!StackIsInitialised(stack))
        return 0;
    free(stack->items);
    stack->initialised = 0;
    stack->capacity = 0;
    stack->size = 0;
    return 1;
}

int StackIsInitialised(Stack *stack)
{
    return stack->initialised;
}

int StackSize(Stack *stack)
{
    return StackIsInitialised(stack)?stack->size:-1;
}

int StackCapacity(Stack *stack)
{
    return StackIsInitialised(stack)?stack->capacity:-1;
}

int StackIsFull(Stack *stack)
{
    return StackIsInitialised(stack)?stack->size>=stack->capacity:0;
}

int StackIsEmpty(Stack *stack)
{
    return StackIsInitialised(stack)?stack->size<=0:-1;
}

int StackPush(Stack *stack, int data)
{
    if (!StackIsInitialised(stack) || StackIsFull(stack))
        return 0;

    stack->items[stack->size++] = data;
    return 1;
}

int StackPull(Stack *stack, int *data)
{
    if (!StackIsInitialised(stack) || StackIsEmpty(stack))
        return 0;

    *data = stack->items[--stack->size];
    return 1;
}

int StackPeek(Stack *stack, int *peek)
{
    if (!StackIsInitialised(stack) || StackIsEmpty(stack))
        return 0;

    *peek = stack->items[stack->size-1];
    return 1;
}
