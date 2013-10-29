/* $Id: miniupnpctypes.h,v 1.2 2012/09/27 15:42:10 nanard Exp $ */
/* Miniupnp project : http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org
 * Author : Thomas Bernard
 * Copyright (c) 2011 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided within this distribution */
#ifndef MINIUPNPCTYPES_H_INCLUDED
#define MINIUPNPCTYPES_H_INCLUDED

#if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L)
#define UNSIGNED_INTEGER unsigned long long
#define STRTOUI	strtoull
#else
#define UNSIGNED_INTEGER unsigned int
#define STRTOUI	strtoul
#endif

#endif

