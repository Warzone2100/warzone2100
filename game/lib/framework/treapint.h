/*
 * TreapInt.h
 *
 * Internal function calls for the treap system to allow it
 * to be used by the memory system.
 *
 */
#ifndef _treapint_h
#define _treapint_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

/* Recursive function to add an object to a tree */
extern void treapAddNode(TREAP_NODE **ppsRoot, TREAP_NODE *psNew, TREAP_CMP cmp);

/* Recursively find and remove a node from the tree */
extern TREAP_NODE *treapDelRec(TREAP_NODE **ppsRoot, UDWORD key,
							   TREAP_CMP cmp);

/* Recurisvely find an object in a treap */
extern void *treapFindRec(TREAP_NODE *psRoot, UDWORD key, TREAP_CMP cmp);

/* Recursively display the treap structure */
extern void treapDisplayRec(TREAP_NODE *psRoot, UDWORD indent);

#endif

