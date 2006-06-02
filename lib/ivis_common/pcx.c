#include <png.h>
#include <setjmp.h>
#include "lib/framework/frame.h"
#include "ivisdef.h"
#include "fbf.h"
#include "bug.h"

#include "ivispatch.h"

typedef struct {
	png_size_t length;
	char* buffer;
} wzpng_io_buf;

static void wzpng_read_data(png_structp ctx, png_bytep area, png_size_t size)
{

	wzpng_io_buf* buf = (wzpng_io_buf*)png_get_io_ptr(ctx);

	if (size <= buf->length) {
		memcpy(area, buf->buffer, size);
		buf->buffer += size;
		buf->length -= size;
	}
}

iBool pie_PNGLoadMem(int8 *pngimage, iSprite *s, iColour *pal)
{
	unsigned int PNG_BYTES_TO_CHECK;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	assert(pngimage != NULL);

	wzpng_io_buf* buf = (wzpng_io_buf*)malloc(sizeof(wzpng_io_buf));
	buf->buffer = (char*)pngimage;
	buf->length = 10000000;

	PNG_BYTES_TO_CHECK = 4;

	if (png_sig_cmp((png_byte*)pngimage, (png_size_t)0, PNG_BYTES_TO_CHECK)) {
		debug(LOG_3D, "pie_PNGLoadMem: Did not recognize PNG header in buffer");
		goto error;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					 NULL, NULL, NULL);

	if (png_ptr == NULL) {
		debug(LOG_3D, "pie_PNGLoadMem: Unable to create png struct");
		goto error;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL) {
		debug(LOG_3D, "pie_PNGLoadMem: Unable to create png info struct");
		goto error;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		debug(LOG_3D, "pie_PNGLoadMem: Error decoding PNG data");
		goto error;
	} else {
		int bit_depth, color_type, interlace_type;
		png_uint_32 width, height;

		/* Set up the input control */
		png_set_read_fn(png_ptr, buf, wzpng_read_data);

		/* Read PNG header info */
		png_read_info(png_ptr, info_ptr);
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
			     &color_type, &interlace_type, NULL, NULL);

		/* tell libpng to strip 16 bit/color files down to 8 bits/color */
		png_set_strip_16(png_ptr) ;

		/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
		 * byte into separate bytes (useful for paletted and grayscale images).
		 */
		png_set_packing(png_ptr);

		/* More transformations to ensure we end up with 32bpp, 4 channel RGBA */
		png_set_gray_to_rgb(png_ptr);
		png_set_palette_to_rgb(png_ptr);
		png_set_tRNS_to_alpha(png_ptr);
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
		png_set_gray_1_2_4_to_8(png_ptr);

		/* scale greyscale values to the range 0..255 */
		if(color_type == PNG_COLOR_TYPE_GRAY)
			png_set_expand(png_ptr);

		png_read_update_info(png_ptr, info_ptr);

		{
			png_uint_32 w, h;

			png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)(&w),
				     (png_uint_32*)(&h), &bit_depth,
				     &color_type, &interlace_type, NULL, NULL);

			s->width = w;
			s->height = h;
			s->bmp = malloc(w*h*info_ptr->channels); // FIXME This is a source for a MEMLEAK!!! (according to Valgrind)
		}

		{
			png_bytep* row_pointers = (png_bytep*)malloc(s->height*sizeof(png_bytep));
			unsigned char* pdata;
			unsigned int i;
			const unsigned int line_size = s->width*info_ptr->channels;

			for (i = 0, pdata = s->bmp;
			     i < s->height;
			     ++i, pdata += line_size) {
				row_pointers[i] = (png_bytep)pdata;
			}

			/* Read the entire image in one go */
			png_read_image(png_ptr, row_pointers);

			free(row_pointers);

			/* read rest of file, get additional chunks in info_ptr - REQUIRED */
			png_read_end(png_ptr, info_ptr);
		}
	}

	if (info_ptr) png_destroy_info_struct(png_ptr, &info_ptr);
	if (png_ptr) png_destroy_read_struct(&png_ptr, NULL, NULL);
	free(buf);

	return TRUE;

error:
	if (info_ptr) png_destroy_info_struct(png_ptr, &info_ptr);
	if (png_ptr) png_destroy_read_struct(&png_ptr, NULL, NULL);
	free(buf);

	return FALSE;
}

BOOL pie_PNGLoadMemToBuffer(int8 *pngimage, iSprite *s, iColour *pal)
{
	return pie_PNGLoadMem(pngimage, s, pal);
}

