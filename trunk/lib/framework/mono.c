/*
 * Mono.c
 *
 * Output to the mono screen
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "mono.h"
#include "types.h"

#define	MONO_SCREEN_ADDR	0xB0000

#define	SLASH_T				0x09						/* tab */
#define	SLASH_N				0x0A						/* new line */

static void	DBug_DumpString(SDWORD, SDWORD, UBYTE *, UBYTE);
static UBYTE	DBug_ExpandString(UBYTE *, UBYTE *, UBYTE, va_list, UBYTE);
static UBYTE	DBug_CheckFormatChar(UBYTE);

/*
*	NAME
*		Debug_MONO_ClearRectangle -- clear a rectangular area
*
*	SYNOPSIS
*		Debug_MONO_ClearRectangle(leftedge, topedge, width, height);
*
*		void	Debug_MONO_ClearRectangle(UBYTE, UBYTE, UBYTE, UBYTE);
*
*	FUNCTION
*		This function clears a rectangular area of the monochrome screen defined by
*		<leftedge>, <topedge> and with the dimensions of <width> x <height>.
*
*/

void	dbg_MONO_ClearRectangle(SDWORD	leftedge,
								SDWORD	topedge,
								SDWORD	width,
								SDWORD	height)
{
	UBYTE	*pub_rectchar,
			 ub_newwidth, ub_newheight,
			 ub_i, ub_j,
			 ub_leftedge, ub_topedge,
			 ub_width, ub_height;

	ub_leftedge = (UBYTE)leftedge;
	ub_topedge = (UBYTE)topedge;
	ub_width = (UBYTE)width;
	ub_height = (UBYTE)height;

	/* do some clipping first */
	ub_newwidth = ub_width;
	ub_newheight = ub_height;

	if ((ub_leftedge + ub_width) > MONO_SCREEN_WIDTH)
	{
		ub_newwidth = MONO_SCREEN_WIDTH - ub_leftedge;
	}
	if ((ub_topedge + ub_height) > MONO_SCREEN_HEIGHT)
	{
		ub_newheight = MONO_SCREEN_HEIGHT - ub_topedge;
	}

	/* see if it's worth doing */
	if ((ub_leftedge < MONO_SCREEN_WIDTH) && (ub_topedge < MONO_SCREEN_HEIGHT) && (ub_newwidth > 2) && (ub_newheight > 2))
	{
		/* get the screen address of the upper-left corner of the box */
		pub_rectchar = (UBYTE *)(MONO_SCREEN_ADDR + ((ub_topedge * (MONO_SCREEN_WIDTH * 2)) + (ub_leftedge * 2)));

		/* clear the rectangle */
		for (ub_i = 0; ub_i < (ub_newheight - 1); ub_i++)
		{
			for (ub_j = 0; ub_j < (ub_newwidth * 2); ub_j++)
			{
				*pub_rectchar++ = '\0';
			}

			/* go to the start of the next line */
			pub_rectchar += ((MONO_SCREEN_WIDTH * 2) - (ub_newwidth * 2));
		}
	}
}

/*
*
*	NAME
*		Debug_MONO_SClearScreen -- clear the screen
*
*	SYNOPSIS
*		Debug_MONO_ClearScreen();
*
*		void	Debug_MONO_ClearScreen(void);
*
*	FUNCTION
*		This routine clears the entire monochrome screen.
*
*/


void	dbg_MONO_ClearScreen(void)
{
	memset((char *)MONO_SCREEN_ADDR, '\0', (MONO_SCREEN_WIDTH * 2) * MONO_SCREEN_HEIGHT);
}

/*
*
*	NAME
*		Debug_MONO_PrintString -- print a formatted string
*
*	SYNOPSIS
*		Debug_MONO_PrintString(leftedge, topedge, string);
*
*		void	Debug_MONO_PrintString(UBYTE, UBYTE, UBYTE *, ...);
*
*	FUNCTION
*		Debug_MONO_PrintString dumps a printf() style NULL-terminated string to the
*		monochrome screen at <leftedge>, <topedge>.
*
*		To set the attribute, use %a anywhere in the format string followed by an
*		attribute byte (defined in debuglib.h) in the argument list.  The default
*		attribute is MONO_NORMAL.
*
*/

void	dbg_MONO_PrintString(SDWORD	 ub_leftedge,
							 SDWORD	 ub_topedge,
							   SBYTE	*pub_formatstring,
							   ...)
{
	va_list	 val_arglist;
	UBYTE	*pub_formatstringptr,
			 aub_percentstring[256],
			 ub_percentstringindex,
			 aub_currentcharacter[2],
			 ub_attribute,
			 aub_expandedstring[256],
			 ub_numprinted;
	BOOL	 bool_percent,
			 bool_printchar;


	/* initialise the variables */
	pub_formatstringptr = (UBYTE*)pub_formatstring;
	aub_percentstring[0] = '\0';
	ub_percentstringindex = 0;
	aub_currentcharacter[1] = '\0';
	ub_attribute = MONO_NORMAL;
	aub_expandedstring[0] = '\0';
	ub_numprinted = 0;
	bool_percent = FALSE;
	bool_printchar = FALSE;

	/* get the first var arg */
	va_start(val_arglist, pub_formatstring);

	/* is there a format string? */
	if (pub_formatstringptr)
	{
		/* yes, there is a format string; process it */
		while (*pub_formatstringptr)
		{
			/* get the next character */
			aub_currentcharacter[0] = *pub_formatstringptr++;

			/* is the character a %? */
			if (aub_currentcharacter[0] == '%')
			{
				/* yes, the character is a %; is the percent flag set? */
				if (bool_percent == TRUE)
				{
					/* yes, the percent flag is set; check for a '%%' case */
					if (ub_percentstringindex == 1)
					{
						aub_percentstring[ub_percentstringindex++] = aub_currentcharacter[0];
						aub_percentstring[ub_percentstringindex] = '\0';

						/* start a new percent string (sort of) */
						ub_percentstringindex = 0;
						bool_percent = FALSE;
					}

					/* expand the percent string */
					ub_attribute = DBug_ExpandString(&aub_expandedstring[0], &aub_percentstring[0], ub_attribute, val_arglist, ub_numprinted);

					/* print the string */
					DBug_DumpString(ub_leftedge, ub_topedge, &aub_expandedstring[0], ub_attribute);
					ub_leftedge += (UBYTE)strlen((const char*)aub_expandedstring);
					ub_numprinted += (UBYTE)strlen((const char*)aub_expandedstring);

					/* start a new percent string; don't do this for a '%%' */
					if (ub_percentstringindex != 0)
					{
						ub_percentstringindex = 0;

						aub_percentstring[ub_percentstringindex++] = aub_currentcharacter[0];
						aub_percentstring[ub_percentstringindex] = '\0';
					}
					else
					{
						/* clear the strings after a '%%' */
						aub_expandedstring[0] = '\0';
						aub_percentstring[0] = '\0';
					}
				}
				else
				{
					/* no, the percent flag isn't set; set it */
					bool_percent = TRUE;

					/* start a new percent string */
					ub_percentstringindex = 0;
					aub_percentstring[ub_percentstringindex++] = aub_currentcharacter[0];
					aub_percentstring[ub_percentstringindex] = '\0';
				}
			}
			else
			{
				/* no, the character isn't a %; is the percent flag set? */
				if (bool_percent == TRUE)
				{
					/* yes, the percent flag is set; put the character in the percent string */
					aub_percentstring[ub_percentstringindex++] = aub_currentcharacter[0];
					aub_percentstring[ub_percentstringindex] = '\0';

					/* is the character a special format character? */
					if (DBug_CheckFormatChar(aub_currentcharacter[0]))
					{
						/* yes, the character is a special format character */
						ub_attribute = DBug_ExpandString(aub_expandedstring, aub_percentstring, ub_attribute, val_arglist, ub_numprinted);

						/* print the string */
						DBug_DumpString(ub_leftedge, ub_topedge, aub_expandedstring, ub_attribute);
						ub_leftedge += (UBYTE)strlen((const char*)aub_expandedstring);
						ub_numprinted += (UBYTE)strlen((const char*)aub_expandedstring);

						/* clear the strings */
						ub_percentstringindex = 0;
						aub_expandedstring[0] = '\0';
						aub_percentstring[0] = '\0';

						/* reset the percent flag */
						bool_percent = FALSE;
					}
				}
				else
				{
					/* no, the percent flag isn't set; is the character a special slash character? */
					switch (aub_currentcharacter[0])
					{
						case	SLASH_T:

							ub_leftedge += (8 - (ub_leftedge % 8));
							ub_numprinted += (8 - (ub_leftedge % 8));
							bool_printchar = FALSE;

							break;


						case	SLASH_N:

							bool_printchar = FALSE;

							break;


						default:

							bool_printchar = TRUE;

							break;
					}

					/* print the character (maybe) */
					if (bool_printchar == TRUE)
					{
						DBug_DumpString(ub_leftedge++, ub_topedge, &aub_currentcharacter[0], ub_attribute);
						ub_numprinted++;
					}

					/* reset the percent flag */
					bool_percent = FALSE;
				}
			}
		}
	}

	/* do one last check for the percent flag; there may be a string not expanded yet */
	if (bool_percent == TRUE)
	{
		/* there is an unxepanded string; expand it */
		ub_attribute = DBug_ExpandString(&aub_expandedstring[0], &aub_percentstring[0], ub_attribute, val_arglist, ub_numprinted);

		/* print the string */
		DBug_DumpString(ub_leftedge, ub_topedge, &aub_expandedstring[0], ub_attribute);
	}

	/* clean up the var arg handler */
	va_end(val_arglist);
}

static void	DBug_DumpString(SDWORD	 ub_leftedge,
						SDWORD	 ub_topedge,
						UBYTE	*pub_string,
						UBYTE	 ub_attr)
{
	UDWORD	 ul_stringlength,
			 ul_i;
	UBYTE	*pub_stringptr,
			*pub_screenptr;


	/* init the string pointer */
	pub_stringptr = pub_string;

	/* is there a string? */
	if (pub_stringptr != NULL)
	{
		/* there is; is it worth printing? */
		if ((ub_leftedge < MONO_SCREEN_WIDTH) && (ub_topedge < MONO_SCREEN_HEIGHT))
		{
			/* it is; init the variables */
			pub_screenptr = (UBYTE *)(MONO_SCREEN_ADDR + ((ub_topedge * (MONO_SCREEN_WIDTH * 2)) + (ub_leftedge * 2)));

			ul_stringlength = strlen((const char*)pub_stringptr);

			/* check for clipping */
			if ((ub_leftedge + ul_stringlength) > MONO_SCREEN_WIDTH)
			{
				ul_stringlength = MONO_SCREEN_WIDTH - ub_leftedge;
			}

			/* yes there is; print it */
			for (ul_i = 0; ul_i < ul_stringlength; ul_i++)
			{
				*pub_screenptr++ = *pub_stringptr++;
				*pub_screenptr++ = ub_attr;
			}
		}
	}
}

static UBYTE	DBug_ExpandString(UBYTE		*pub_stringbuffer,
						  UBYTE		*pub_percentstring,
						  UBYTE		 ub_oldattribute,
						  va_list	 val_arglist,
						  UBYTE		 ub_numprinted)
{
	UBYTE	 ub_percentchar,
			 ub_newattribute,
			 ub_stringlength,
			*pub_numberchars;


	/* init the variables */
	ub_newattribute = ub_oldattribute;

	/* find the last character */
	ub_stringlength = (UBYTE)strlen((const char*)pub_percentstring) - 1;
	ub_percentchar = *(pub_percentstring + ub_stringlength);

	/* see what it is */
	switch (ub_percentchar)
	{
		case	'a':	/* attribute (char) */
			ub_newattribute = va_arg(val_arglist, int);
// changed because gcc 4 says it'll crash here
// was			ub_newattribute = va_arg(val_arglist, char);

			break;


		case	'c':	/* character */
			sprintf((char*)pub_stringbuffer, (const char*)pub_percentstring, va_arg(val_arglist, int));
// changed because gcc 4 says it'll crash here
// was			sprintf((char*)pub_stringbuffer, (const char*)pub_percentstring, va_arg(val_arglist, char));

			break;


		case	'd':	/* signed decimal integer */
		case	'i':	/* signed decimal integer */
		case	'u':	/* unsigned decimal integers */
		case	'o':	/* unsigned octal */

			sprintf((char*)pub_stringbuffer, (const char*)pub_percentstring, va_arg(val_arglist, int));

			break;


		case	'e':	/* scientific notation (e) */
		case	'E':	/* scientific notation (E) */
		case	'f':	/* decimal floating point */
		case	'g':	/* uses %e or %f, whichever is shorter */
		case	'G':	/* uses %E or %F, whichever is shorter */

			sprintf((char*)pub_stringbuffer, (const char*)pub_percentstring, va_arg(val_arglist, double));

			break;


		case	'p':	/* displays a pointer */
		case	's':	/* string */
		case	'x':	/* unsigned hexadecimal (lower case) */
		case	'X':	/* unsigned hexadecimal (upper case) */

			sprintf((char*)pub_stringbuffer, (const char*)pub_percentstring, va_arg(val_arglist, char *));

			break;


		case	'n':	/* how many characters printed so far */

			pub_numberchars = (UBYTE*)va_arg(val_arglist, char *);
			*pub_numberchars = ub_numprinted;

			break;


		case	'%':	/* percent char (no arg) */

			sprintf((char*)pub_stringbuffer, "%%");

			break;


		default:		/* unknown conversion character */

			break;
	}

	/* return a (possibly new) attribute */
	return (ub_newattribute);
}

static UBYTE	DBug_CheckFormatChar(UBYTE	ub_percentchar)
{
	UBYTE	ub_isformatchar;


	/* init the variable */
	ub_isformatchar = 0;

	/* check to see if it's a format character */
	switch (ub_percentchar)
	{
		case	'a':	/* attribute (char) */
		case	'c':	/* character */
		case	'd':	/* signed decimal integer */
		case	'i':	/* signed decimal integer */
		case	'e':	/* scientific notation (e) */
		case	'E':	/* scientific notation (E) */
		case	'f':	/* decimal floating point */
		case	'g':	/* uses %e or %f, whichever is shorter */
		case	'G':	/* uses %E or %F, whichever is shorter */
		case	'o':	/* unsigned octal */
		case	'p':	/* displays a pointer */
		case	's':	/* string */
		case	'u':	/* unsigned decimal integers */
		case	'x':	/* unsigned hexadecimal (lower case) */
		case	'X':	/* unsigned hexadecimal (upper case) */
		case	'n':	/* how many characters printed so far */
		case	'%':	/* percent char (no arg) */

			ub_isformatchar = 1;

			break;


		default:		/* unknown conversion character */

			ub_isformatchar = 0;

			break;
	}

	return (ub_isformatchar);
}


