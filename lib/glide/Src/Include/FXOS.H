/*
** Copyright (c) 1995, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
** $ Revision: $
** $ Date: $
**
*/


#ifndef _FXOS_H_
#define _FXOS_H_

#include <stdio.h>

#   ifdef __cplusplus
extern "C" {
#   endif

#   ifdef WIN32
void sleep(int secs);
#define gethostname fxGethostname

int gethostname(char *name, int namelen);

#   endif

float fxTime(void);
float timer(int flag);

FILE *fxFopenPath(const char *filename, const char *mode,
					const char *path, const char **pprefix);

#   ifdef __cplusplus
}
#   endif

#endif
