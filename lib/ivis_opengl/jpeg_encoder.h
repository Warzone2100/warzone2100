#ifndef JPEG_H_asdf
#define JPEG_H_asdf

/*
 * unsigned char *encode_image(unsigned char *input_ptr, unsigned char *output_ptr, unsigned int quality_factor,
 *                             unsigned int image_format, unsigned int image_width, unsigned int image_height)
 * Example:
 * output_end = encode_image(input, output_start, quality, JPEG_FORMAT_FOUR_TWO_TWO, 320, 256);
 *
 * output_end - output_start gives the size of the JPEG data.
 *
 */
unsigned char *jpeg_encode_image(unsigned char *, unsigned char *, unsigned int, unsigned int, unsigned int, unsigned int);

#define        JPEG_FORMAT_FOUR_ZERO_ZERO          0
#define        JPEG_FORMAT_FOUR_TWO_TWO            2
#define        JPEG_FORMAT_RGB                     4

#endif
