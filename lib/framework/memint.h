/*! \file memint.h
 *  \brief Internal framework definitions for the memory system.
 */
#ifndef _memint_h
#define _memint_h

/* The size of saftey buffer to leave before and after any malloc'ed memory.
   Can use this to check writing over end of buffers */

#define SAFETY_ZONE_SIZE	(32)


/* The character used to initialise malloc'ed memory, or to trash memory before
   freeing it */
#define SAFETY_ZONE_BYTE	(0xac)
#define INITIALISE_BYTE		(0xcd)
#define FREE_BYTE			(0xdd)

// memory block header for the treap code
typedef struct _mem_node
{
	TREAP_NODE_BASE;		// The treap data to store the node
	TREAP_NODE_DEBUG;		// The debug info for the node (file, line).
	UDWORD		size;		// The memory block size
} MEM_NODE;

/* compare two memory blocks */
extern SDWORD	memBlockCmp(void *key1, void *key2);
#endif

