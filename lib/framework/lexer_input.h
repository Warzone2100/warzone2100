#ifndef __INCLUDED_LIB_FRAMEWORK_LEXER_INPUT_H__
#define __INCLUDED_LIB_FRAMEWORK_LEXER_INPUT_H__

#include <physfs.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

enum lexinput_type
{
	LEXINPUT_STDIO,
	LEXINPUT_PHYSFS,
	LEXINPUT_BUFFER,
};

typedef struct
{
	union
	{
		FILE* stdiofile;
		PHYSFS_file* physfsfile;
		struct
		{
			const char* begin;
			const char* end;
		} buffer;
	} input;
	enum lexinput_type type;
} lexerinput_t;

#ifdef YY_EXTRA_TYPE
# undef YY_EXTRA_TYPE
#endif

#define YY_EXTRA_TYPE lexerinput_t *

extern int lexer_input(lexerinput_t* input, char* buf, size_t max_size, int nullvalue);

#define YY_INPUT(buf, result, max_size) \
do \
{ \
	result = lexer_input(yyextra, buf, max_size, YY_NULL); \
} while(0)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_LIB_FRAMEWORK_LEXER_INPUT_H__
