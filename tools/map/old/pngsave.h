#ifndef PNGSAVE_H
#define PNGSAVE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The pixels argument should such that
 * pixels[0] is the bottom left corner and
 * pixels[h-1] is the top left corner
 * Pixel components which exceed one byte are expected in network byte order.
 */
bool savePng(const char *filename, uint8_t *pixels, int w, int h);
bool savePngARGB32(const char *filename, uint8_t *pixels, int w, int h);
bool savePngI16(const char *filename, uint16_t *pixels, int w, int h);
bool savePngI8(const char *filename, uint8_t *pixels, int w, int h);

#endif
