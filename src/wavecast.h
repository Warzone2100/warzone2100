#ifndef _WAVE_CAST_H_
#define _WAVE_CAST_H_

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef struct WavecastTile
{
	int16_t dx, dy;            ///< Tile coordinates.
	int32_t invRadius;         ///< Arbitrary constant divided by radius.
	int16_t angBegin, angEnd;  ///< Start and finish angles for obstruction of view. Non-linear units, for comparison purposes only.
} WAVECAST_TILE;


// Not thread safe if someone calls with a new radius. Thread safe, otherwise.
const WAVECAST_TILE *getWavecastTable(unsigned radius, size_t *size);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WAVE_CAST_H_
