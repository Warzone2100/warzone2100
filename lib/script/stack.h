/*
 * Stack.h
 *
 * Interface to the stack system
 */
#ifndef _stack_h
#define _stack_h

/* Initialise the stack */
extern BOOL stackInitialise(void);

/* Shutdown the stack */
extern void stackShutDown(void);

/* Push a value onto the stack */
extern BOOL stackPush(INTERP_VAL  *psVal);

/* Pop a value off the stack */
extern BOOL stackPop(INTERP_VAL  *psVal);

/* Pop a value off the stack, checking that the type matches what is passed in */
extern BOOL stackPopType(INTERP_VAL  *psVal);

/* Look at a value on the stack without removing it.
 * index is how far down the stack to look.
 * Index 0 is the top entry on the stack.
 */
extern BOOL stackPeek(INTERP_VAL *psVal, UDWORD index);

/* Print the top value on the stack */
extern void stackPrintTop(void);

/* Check if the stack is empty */
extern BOOL stackEmpty(void);

/* Do binary operations on the top of the stack
 * This effectively pops two values and pushes the result
 */
extern BOOL stackBinaryOp(OPCODE opcode);

/* Perform a unary operation on the top of the stack
 * This effectively pops a value and pushes the result
 */
extern BOOL stackUnaryOp(OPCODE opcode);

/* Reset the stack to an empty state */
extern void stackReset(void);



#endif
