/*
 * CodePrint.h
 *
 * Routines for displaying compiled scripts
 *
 */
#ifndef _codeprint_h
#define _codeprint_h

/* Display a value type */
extern void cpPrintType(INTERP_TYPE type);

/* Display a value  */
extern void cpPrintVal(INTERP_VAL *psVal);

/* Display a value from a program that has been packed with an opcode */
extern void cpPrintPackedVal(UDWORD *ip);

/* Print a function name */
extern void cpPrintFunc(SCRIPT_FUNC pFunc);

/* Print a variable access function name */
extern void cpPrintVarFunc(SCRIPT_VARFUNC pFunc, UDWORD index);

/* Display a maths operator */
extern void cpPrintMathsOp(UDWORD opcode);

#endif

