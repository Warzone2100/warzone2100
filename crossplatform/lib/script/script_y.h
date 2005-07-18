typedef union {
	/* Types returned by the lexer */
	BOOL			bval;
	/*	float			fval; */
	SDWORD			ival;
	STRING			*sval;
	INTERP_TYPE		tval;
	STORAGE_TYPE	stype;
	VAR_SYMBOL		*vSymbol;
	CONST_SYMBOL	*cSymbol;
	FUNC_SYMBOL		*fSymbol;
	TRIGGER_SYMBOL	*tSymbol;
	EVENT_SYMBOL	*eSymbol;
	CALLBACK_SYMBOL	*cbSymbol;

	/* Types only returned by rules */
	CODE_BLOCK		*cblock;
	COND_BLOCK		*condBlock;
	OBJVAR_BLOCK	*objVarBlock;
	ARRAY_BLOCK		*arrayBlock;
	PARAM_BLOCK		*pblock;
	PARAM_DECL		*pdecl;
	TRIGGER_DECL	*tdecl;
	VAR_DECL		*vdecl;
	VAR_IDENT_DECL	*videcl;
} YYSTYPE;
#define FUNCTION	257
#define TRIGGER	258
#define EVENT	259
#define WAIT	260
#define EVERY	261
#define INACTIVE	262
#define INITIALISE	263
#define LINK	264
#define REF	265
#define WHILE	266
#define IF	267
#define ELSE	268
#define EXIT	269
#define PAUSE	270
#define BOOLEQUAL	271
#define NOTEQUAL	272
#define GREATEQUAL	273
#define LESSEQUAL	274
#define GREATER	275
#define LESS	276
#define _AND	277
#define _OR	278
#define _NOT	279
#define UMINUS	280
#define BOOLEAN	281
#define INTEGER	282
#define QTEXT	283
#define TYPE	284
#define STORAGE	285
#define IDENT	286
#define VAR	287
#define BOOL_VAR	288
#define NUM_VAR	289
#define OBJ_VAR	290
#define VAR_ARRAY	291
#define BOOL_ARRAY	292
#define NUM_ARRAY	293
#define OBJ_ARRAY	294
#define BOOL_OBJVAR	295
#define NUM_OBJVAR	296
#define USER_OBJVAR	297
#define OBJ_OBJVAR	298
#define BOOL_CONSTANT	299
#define NUM_CONSTANT	300
#define USER_CONSTANT	301
#define OBJ_CONSTANT	302
#define FUNC	303
#define BOOL_FUNC	304
#define NUM_FUNC	305
#define USER_FUNC	306
#define OBJ_FUNC	307
#define TRIG_SYM	308
#define EVENT_SYM	309
#define CALLBACK_SYM	310
extern YYSTYPE scr_lval;
