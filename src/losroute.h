/*
 * LOSRoute.h
 *
 * Route finding code
 *
 */
#ifndef _losroute_h
#define _losroute_h

// Find a route through the map on a tile basis
extern BOOL fpathTileRoute(MOVE_CONTROL *psMoveCntl,
					SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy);

#endif


