#ifndef __INCLUDED_SRC_PARSETEST_H__
#define __INCLUDED_SRC_PARSETEST_H__

typedef struct
{
	const char*     testname;
	const char*     content;
} test_files;

extern void parseTest(void);

#endif // __INCLUDED_SRC_PARSETEST_H__
