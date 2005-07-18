#ifndef __ADPCM_H__
#define __ADPCM_H__

void adpcm_init();
void adpcm_decode(unsigned char* input, unsigned int input_size, short** output);

#endif

