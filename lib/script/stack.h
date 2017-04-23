/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * Stack.h
 *
 * Interface to the stack system
 */
#ifndef _stack_h
#define _stack_h

#include "interpreter.h"

//String support
//-----------------------------
#define MAXSTRLEN	255					//Max len of a single string
#define MAXSTACKLEN	8000

/* Initialise the stack */
extern bool stackInitialise(void);

/* Shutdown the stack */
extern void stackShutDown(void);

/* Push a value onto the stack */
extern bool stackPush(INTERP_VAL  *psVal);

/* Pop a value off the stack */
extern bool stackPop(INTERP_VAL  *psVal);

/* Return pointer to the top value without poping it */
extern bool stackPeekTop(INTERP_VAL  **ppsVal);

/* Pop a value off the stack, checking that the type matches what is passed in */
extern bool stackPopType(INTERP_VAL  *psVal);

/* Look at a value on the stack without removing it.
 * index is how far down the stack to look.
 * Index 0 is the top entry on the stack.
 */
extern bool stackPeek(INTERP_VAL *psVal, UDWORD index);

/* Print the top value on the stack */
extern void stackPrintTop(void);

/* Check if the stack is empty */
extern bool stackEmpty(void);

/* Do binary operations on the top of the stack
 * This effectively pops two values and pushes the result
 */
extern bool stackBinaryOp(OPCODE opcode);

/* Perform a unary operation on the top of the stack
 * This effectively pops a value and pushes the result
 */
extern bool stackUnaryOp(OPCODE opcode);

/* casts top of the stack to some other data type */
extern bool stackCastTop(INTERP_TYPE neededType);

/* Reset the stack to an empty state */
extern void stackReset(void);

#endif
