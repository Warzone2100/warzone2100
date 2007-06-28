typedef union {
	float		fval;
	long		ival;
	signed char	bval;
	char		sval[100];
} YYSTYPE;
#define FLOAT	257
#define INTEGER	258
#define TEXT	259
#define text	260
#define QTEXT	261
#define LOOP	262
#define ONESHOT	263
#define AUDIO	264
#define ANIM2D	265
#define ANIM3DFRAMES	266
#define ANIM3DTRANS	267
#define ANIM3DFILE	268
#define AUDIO_MODULE	269
#define ANIM_MODULE	270
#define ANIMOBJECT	271
extern YYSTYPE audp_lval;
