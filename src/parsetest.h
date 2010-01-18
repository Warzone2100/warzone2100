#ifndef __INCLUDED_SRC_PARSETEST_H__
#define __INCLUDED_SRC_PARSETEST_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef struct
{
	const char*     testname;
	const char*     content;
} test_files;

extern void parseTest(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_PARSETEST_H__
