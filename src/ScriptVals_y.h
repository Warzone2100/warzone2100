typedef union {
	BOOL			bval;
	INTERP_TYPE		tval;
	STRING			*sval;
	UDWORD			vindex;
	SDWORD			ival;
	VAR_INIT		sInit;
	ARRAY_INDEXES	*arrayIndex;
} YYSTYPE;
#define BOOLEAN	257
#define INTEGER	258
#define IDENT	259
#define QTEXT	260
#define TYPE	261
#define VAR	262
#define ARRAY	263
#define SCRIPT	264
#define STORE	265
#define RUN	266
extern YYSTYPE scrv_lval;
