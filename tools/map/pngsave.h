#ifndef PNGSAVE_H
#define PNGSAVE_H

#include <stdbool.h>
#include <stdint.h>

bool savePng(const char *filename, char *pixels, int w, int h);
bool savePngI16(const char *filename, uint16_t *pixels, int w, int h);
bool savePngI8(const char *filename, uint8_t *pixels, int w, int h);

#endif
