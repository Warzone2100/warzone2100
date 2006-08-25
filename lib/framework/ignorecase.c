/** \file ignorecase.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <physfs.h>
#include "ignorecase.h"

/**
 * Please see ignorecase.h for details.
 *
 * License: this code is public domain. I make no warranty that it is useful,
 *  correct, harmless, or environmentally safe.
 *
 * This particular file may be used however you like, including copying it
 *  verbatim into a closed-source project, exploiting it commercially, and
 *  removing any trace of my name from the source (although I hope you won't
 *  do that). I welcome enhancements and corrections to this file, but I do
 *  not require you to send me patches if you make changes. This code has
 *  NO WARRANTY.
 *
 * Unless otherwise stated, the rest of PhysicsFS falls under the zlib license.
 *  Please see LICENSE in the root of the source tree.
 *
 *  \author Ryan C. Gordon.
 */

/* I'm not screwing around with stricmp vs. strcasecmp... */
static int caseInsensitiveStringCompare(const char *x, const char *y)
{
    int ux, uy;
    do
    {
        ux = toupper((int) *x);
        uy = toupper((int) *y);
        if (ux != uy)
            return((ux > uy) ? 1 : -1);
        x++;
        y++;
    } while ((ux) && (uy));

    return(0);
} /* caseInsensitiveStringCompare */


static int locateOneElement(char *buf)
{
    char *ptr;
    char **rc;
    char **i;

    if (PHYSFS_exists(buf))
        return(1);  /* quick rejection: exists in current case. */

    ptr = strrchr(buf, '/');  /* find entry at end of path. */
    if (ptr == NULL)
    {
        rc = PHYSFS_enumerateFiles("/");
        ptr = buf;
    } /* if */
    else
    {
        *ptr = '\0';
        rc = PHYSFS_enumerateFiles(buf);
        *ptr = '/';
        ptr++;  /* point past dirsep to entry itself. */
    } /* else */

    for (i = rc; *i != NULL; i++)
    {
        if (caseInsensitiveStringCompare(*i, ptr) == 0)
        {
            strcpy(ptr, *i); /* found a match. Overwrite with this case. */
            PHYSFS_freeList(rc);
            return(1);
        } /* if */
    } /* for */

    /* no match at all... */
    PHYSFS_freeList(rc);
    return(0);
} /* locateOneElement */


int PHYSFSEXT_locateCorrectCase(char *buf)
{
    int rc;
    char *ptr;
    char *prevptr;

    while (*buf == '/')  /* skip any '/' at start of string... */
        buf++;

    ptr = prevptr = buf;
    if (*ptr == '\0')
        return(0);  /* Uh...I guess that's success. */

    while ((ptr = strchr(ptr + 1, '/')))
    {
        *ptr = '\0';  /* block this path section off */
        rc = locateOneElement(buf);
        *ptr = '/'; /* restore path separator */
        if (!rc)
            return(-2);  /* missing element in path. */
    } /* while */

    /* check final element... */
    return(locateOneElement(buf) ? 0 : -1);
} /* PHYSFSEXT_locateCorrectCase */

/* end of ignorecase.c ... */

