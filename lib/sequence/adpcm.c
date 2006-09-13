#include "adpcm.h"

char index_adjust[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

short step_size[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

int pred_val = 0;
int step_idx = 0;

/* This code is "borrowed" from the ALSA library 
   http://www.alsa-project.org */
short adpcm_decode_sample(char code) {
	short pred_diff;	/* Predicted difference to next sample */
	short step;		/* holds previous step_size value */
	char sign;

	int i;

	/* Separate sign and magnitude */
	sign = code & 0x8;
	code &= 0x7;

	/*
	 * Computes pred_diff = (code + 0.5) * step / 4,
	 * but see comment in adpcm_coder.
	 */

	step = step_size[step_idx];

	/* Compute difference and new predicted value */
	pred_diff = step >> 3;
	for (i = 0x4; i; i >>= 1, step >>= 1) {
		if (code & i) {
			pred_diff += step;
		}
	}
	pred_val += (sign) ? -pred_diff : pred_diff;

	/* Clamp output value */
	if (pred_val > 32767) {
		pred_val = 32767;
	} else if (pred_val < -32768) {
		pred_val = -32768;
	}

	/* Find new step_size index value */
	step_idx += index_adjust[(int) code];

	if (step_idx < 0) {
		step_idx = 0;
	} else if (step_idx > 88) {
		step_idx = 88;
	}
	return pred_val;
}

void adpcm_init(void) {
	pred_val = 0;
	step_idx = 0;
}

void adpcm_decode(unsigned char* input, unsigned int input_size, short** output) {
	unsigned int i;

	for (i = 0; i < input_size; ++i) {
		unsigned char two_samples = input[i];
		*((*output)++) = adpcm_decode_sample(two_samples >> 4);
		*((*output)++) = adpcm_decode_sample(two_samples & 15);
	}
}
