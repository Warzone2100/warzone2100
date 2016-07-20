/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  jpeg.c - JPEG compression for SRV-1 robot
*    Copyright (C) 2005-2009  Surveyor Corporation
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details (www.gnu.org/licenses)
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//
// This file contains optimizations and other changes (C) Frank Van Hooft 2009
//
// NOTE THIS FILE IS CURRENTLY USING THE C DCT VERSION. THE ASSEMBLER DCT IS
// CURRENTLY PRODUCING SOME WIERD RESULTS IN CERTAIN CONDITIONS, WHERE TEXT IS
// PLACED ON THE IMAGE. Even without text, the asm dct produces a "blockier"
// looking image; the c dct produces a smoother finer-grained image.

// Cleaned up and removed the assembly for use in Warzone.

#include "jpeg_encoder.h"

#include <assert.h>

typedef char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;

typedef struct JPEG_ENCODER_STRUCTURE
{
	UINT16 mcu_width;
	UINT16 mcu_height;
	UINT16 horizontal_mcus;
	UINT16 vertical_mcus;
	UINT16 cols_in_right_mcus;
	UINT16 rows_in_bottom_mcus;

	UINT16 rows;
	UINT16 cols;

	UINT16 length_minus_mcu_width;
	UINT16 length_minus_width;
	UINT16 incr;
	UINT16 mcu_width_size;
	UINT16 offset;

	INT16 ldc1;
	INT16 ldc2;
	INT16 ldc3;
} JPEG_ENCODER_STRUCTURE;


#define        BLOCK_SIZE                64


UINT16 luminance_dc_code_table[] =
{
	0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
	0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
};

UINT16 luminance_dc_size_table[] =
{
	0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
};

UINT16 chrominance_dc_code_table[] =
{
	0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
	0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
};

UINT16 chrominance_dc_size_table[] =
{
	0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005,
	0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
};

UINT16 luminance_ac_code_table[] =
{
	0x000A,
	0x0000, 0x0001, 0x0004, 0x000B, 0x001A, 0x0078, 0x00F8, 0x03F6, 0xFF82, 0xFF83,
	0x000C, 0x001B, 0x0079, 0x01F6, 0x07F6, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88,
	0x001C, 0x00F9, 0x03F7, 0x0FF4, 0xFF89, 0xFF8A, 0xFF8b, 0xFF8C, 0xFF8D, 0xFF8E,
	0x003A, 0x01F7, 0x0FF5, 0xFF8F, 0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95,
	0x003B, 0x03F8, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D,
	0x007A, 0x07F7, 0xFF9E, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5,
	0x007B, 0x0FF6, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD,
	0x00FA, 0x0FF7, 0xFFAE, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5,
	0x01F8, 0x7FC0, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD,
	0x01F9, 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6,
	0x01FA, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
	0x03F9, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8,
	0x03FA, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1,
	0x07F8, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA,
	0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4,
	0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
	0x07F9
};

UINT16 luminance_ac_size_table[] =
{
	0x0004,
	0x0002, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 0x000A, 0x0010, 0x0010,
	0x0004, 0x0005, 0x0007, 0x0009, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0005, 0x0008, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0006, 0x0009, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0006, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0007, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0008, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000B
};

UINT16 chrominance_ac_code_table[] =
{
	0x0000,
	0x0001, 0x0004, 0x000A, 0x0018, 0x0019, 0x0038, 0x0078, 0x01F4, 0x03F6, 0x0FF4,
	0x000B, 0x0039, 0x00F6, 0x01F5, 0x07F6, 0x0FF5, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B,
	0x001A, 0x00F7, 0x03F7, 0x0FF6, 0x7FC2, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F, 0xFF90,
	0x001B, 0x00F8, 0x03F8, 0x0FF7, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96,
	0x003A, 0x01F6, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E,
	0x003B, 0x03F9, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6,
	0x0079, 0x07F7, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE,
	0x007A, 0x07F8, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6,
	0x00F9, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
	0x01F7, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8,
	0x01F8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF, 0xFFD0, 0xFFD1,
	0x01F9, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA,
	0x01FA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3,
	0x07F9, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEb, 0xFFEC,
	0x3FE0, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5,
	0x7FC3, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
	0x03FA
};

UINT16 chrominance_ac_size_table[] =
{
	0x0002,
	0x0002, 0x0003, 0x0004, 0x0005, 0x0005, 0x0006, 0x0007, 0x0009, 0x000A, 0x000C,
	0x0004, 0x0006, 0x0008, 0x0009, 0x000B, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0005, 0x0008, 0x000A, 0x000C, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0005, 0x0008, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0006, 0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0006, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0008, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000E, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
	0x000A
};


UINT8 bitsize[] =
{
	0, 1, 2, 2, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8
};

UINT8 markerdata[] =
{
	0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
	0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
	0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
	0xFF, 0xC4, 0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
};


UINT8 zigzag_table[] =
{
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};
UINT8 luminance_quant_table[] =
{
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

UINT8 chrominance_quant_table[] =
{
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

UINT8 Lqt[BLOCK_SIZE];
UINT8 Cqt[BLOCK_SIZE];
UINT16 ILqt[BLOCK_SIZE];
UINT16 ICqt[BLOCK_SIZE];

INT16 Y1[BLOCK_SIZE];
INT16 Y2[BLOCK_SIZE];
INT16 CB[BLOCK_SIZE];
INT16 CR[BLOCK_SIZE];
INT16 Temp[BLOCK_SIZE];
UINT32 lcode = 0;
UINT16 bitindex = 0;

void (*read_format)(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure, UINT8 *input_ptr);


static void quantization(INT16 *data, UINT8 *zigzag_table, INT16 *Temp, UINT16 *quant_table_ptr)
{
	INT16 i;
	INT32 value;

	for (i = 63; i >= 0; i--)
	{
		value = data[i] * quant_table_ptr[i];
		value = (value + 0x4000) >> 15;

		Temp[zigzag_table[i]] = (INT16) value;
	}
}


// RGB24 format is 3 bytes per pixel, in the order red - green - blue. This function reads two rgb pixels
// at a time (ie 6 pixel bytes), then computes Y for each pixel, and the average of U & V for the two pixels.
// So four values are calculated & saved (Y, Y, U & V).
static void read_rgb24_format(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure, UINT8 *input_ptr)
{
	INT32 i, j;
	UINT16 Y1_cols, Y2_cols;

	INT16 *Y1_Ptr = Y1;
	INT16 *Y2_Ptr = Y2;
	INT16 *CB_Ptr = CB;
	INT16 *CR_Ptr = CR;

	UINT16 rows = jpeg_encoder_structure->rows;
	UINT16 cols = jpeg_encoder_structure->cols;
	UINT16 incr = jpeg_encoder_structure->incr;

	if (cols <= 8)
	{
		Y1_cols = cols;
		Y2_cols = 0;
	}
	else
	{
		Y1_cols = 8;
		Y2_cols = (UINT16)(cols - 8);
	}

	for (i = rows; i > 0; i--)
	{
		for (j = Y1_cols >> 1; j > 0; j--)
		{
			*CB_Ptr++ = ((128 * input_ptr[2] - 85 * input_ptr[1] - 43 * input_ptr[0] + 128 * input_ptr[5] - 85 * input_ptr[4] - 43 * input_ptr[3]) >> 9);
			*Y1_Ptr++ = ((76 * input_ptr[0] + 151 * input_ptr[1] + 29 * input_ptr[2]) >> 8) - 128;
			*CR_Ptr++ = ((128 * input_ptr[0] - 107 * input_ptr[1] - 21 * input_ptr[2] + 128 * input_ptr[3] - 107 * input_ptr[4] - 21 * input_ptr[5]) >> 9);
			*Y1_Ptr++ = ((76 * input_ptr[3] + 151 * input_ptr[4] + 29 * input_ptr[5]) >> 8) - 128;
			input_ptr += 6;
		}

		for (j = Y2_cols >> 1; j > 0; j--)
		{
			*CB_Ptr++ = ((128 * input_ptr[2] - 85 * input_ptr[1] - 43 * input_ptr[0] + 128 * input_ptr[5] - 85 * input_ptr[4] - 43 * input_ptr[3]) >> 9);
			*Y2_Ptr++ = ((76 * input_ptr[0] + 151 * input_ptr[1] + 29 * input_ptr[2]) >> 8) - 128;
			*CR_Ptr++ = ((128 * input_ptr[0] - 107 * input_ptr[1] - 21 * input_ptr[2] + 128 * input_ptr[3] - 107 * input_ptr[4] - 21 * input_ptr[5]) >> 9);
			*Y2_Ptr++ = ((76 * input_ptr[3] + 151 * input_ptr[4] + 29 * input_ptr[5]) >> 8) - 128;
			input_ptr += 6;
		}

		if (cols <= 8)
		{
			for (j = 8 - Y1_cols; j > 0; j--)
			{
				*Y1_Ptr = *(Y1_Ptr - 1);
				Y1_Ptr++;
			}

			for (j = 8 - Y2_cols; j > 0; j--)
			{
				*Y2_Ptr = *(Y1_Ptr - 1);
				Y2_Ptr++;
			}
		}
		else
		{
			for (j = 8 - Y2_cols; j > 0; j--)
			{
				*Y2_Ptr = *(Y2_Ptr - 1);
				Y2_Ptr++;
			}
		}

		for (j = (16 - cols) >> 1; j > 0; j--)
		{
			*CB_Ptr = *(CB_Ptr - 1);  CB_Ptr++;
			*CR_Ptr = *(CR_Ptr - 1);  CR_Ptr++;
		}

		input_ptr += incr;
	}

	for (i = 8 - rows; i > 0; i--)
	{
		for (j = 8; j > 0; j--)
		{
			*Y1_Ptr = *(Y1_Ptr - 8);  Y1_Ptr++;
			*Y2_Ptr = *(Y2_Ptr - 8);  Y2_Ptr++;
			*CB_Ptr = *(CB_Ptr - 8);  CB_Ptr++;
			*CR_Ptr = *(CR_Ptr - 8);  CR_Ptr++;
		}
	}
}


static void initialization(JPEG_ENCODER_STRUCTURE *jpeg, UINT32 image_format, UINT32 image_width, UINT32 image_height)
{
	UINT16 mcu_width, mcu_height, bytes_per_pixel;

	assert(image_format == JPEG_FORMAT_RGB);

	jpeg->mcu_width = mcu_width = 16;
	jpeg->horizontal_mcus = (UINT16)((image_width + mcu_width - 1) >> 4);

	jpeg->mcu_height = mcu_height = 8;
	jpeg->vertical_mcus = (UINT16)((image_height + mcu_height - 1) >> 3);
	bytes_per_pixel = 3;
	read_format = read_rgb24_format;

	jpeg->rows_in_bottom_mcus = (UINT16)(image_height - (jpeg->vertical_mcus - 1) * mcu_height);
	jpeg->cols_in_right_mcus = (UINT16)(image_width - (jpeg->horizontal_mcus - 1) * mcu_width);

	jpeg->length_minus_mcu_width = (UINT16)((image_width - mcu_width) * bytes_per_pixel);
	jpeg->length_minus_width = (UINT16)((image_width - jpeg->cols_in_right_mcus) * bytes_per_pixel);

	jpeg->mcu_width_size = (UINT16)(mcu_width * bytes_per_pixel);

	jpeg->offset = (UINT16)((image_width * (mcu_height - 1) - (mcu_width - jpeg->cols_in_right_mcus)) * bytes_per_pixel);

	jpeg->ldc1 = 0;
	jpeg->ldc2 = 0;
	jpeg->ldc3 = 0;
	lcode = 0;
	bitindex = 0;
}


/* DCT for One block(8x8) */
static void DCT(INT16 *data)
{
	UINT16 i;
	INT32 x0, x1, x2, x3, x4, x5, x6, x7, x8;

	static const UINT16 c1 = 1420; // cos PI/16 * root(2)
	static const UINT16 c2 = 1338; // cos PI/8 * root(2)
	static const UINT16 c3 = 1204; // cos 3PI/16 * root(2)
	static const UINT16 c5 = 805; // cos 5PI/16 * root(2)
	static const UINT16 c6 = 554; // cos 3PI/8 * root(2)
	static const UINT16 c7 = 283; // cos 7PI/16 * root(2)

	static const UINT16 s1 = 3;
	static const UINT16 s2 = 10;
	static const UINT16 s3 = 13;

	for (i = 8; i > 0; i--)
	{
		x8 = data[0] + data[7];
		x0 = data[0] - data[7];

		x7 = data[1] + data[6];
		x1 = data[1] - data[6];

		x6 = data[2] + data[5];
		x2 = data[2] - data[5];

		x5 = data[3] + data[4];
		x3 = data[3] - data[4];

		x4 = x8 + x5;
		x8 = x8 - x5;

		x5 = x7 + x6;
		x7 = x7 - x6;

		data[0] = (INT16)(x4 + x5);
		data[4] = (INT16)(x4 - x5);

		data[2] = (INT16)((x8 * c2 + x7 * c6) >> s2);
		data[6] = (INT16)((x8 * c6 - x7 * c2) >> s2);

		data[7] = (INT16)((x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> s2);
		data[5] = (INT16)((x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> s2);
		data[3] = (INT16)((x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> s2);
		data[1] = (INT16)((x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> s2);

		data += 8;
	}

	data -= 64;

	for (i = 8; i > 0; i--)
	{
		x8 = data[0] + data[56];
		x0 = data[0] - data[56];

		x7 = data[8] + data[48];
		x1 = data[8] - data[48];

		x6 = data[16] + data[40];
		x2 = data[16] - data[40];

		x5 = data[24] + data[32];
		x3 = data[24] - data[32];

		x4 = x8 + x5;
		x8 = x8 - x5;

		x5 = x7 + x6;
		x7 = x7 - x6;

		data[0] = (INT16)((x4 + x5) >> s1);
		data[32] = (INT16)((x4 - x5) >> s1);

		data[16] = (INT16)((x8 * c2 + x7 * c6) >> s3);
		data[48] = (INT16)((x8 * c6 - x7 * c2) >> s3);

		data[56] = (INT16)((x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> s3);
		data[40] = (INT16)((x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> s3);
		data[24] = (INT16)((x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> s3);
		data[8] = (INT16)((x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> s3);

		data++;
	}
}


#define PUTBITS \
	{ \
		bits_in_next_word = (INT16) (bitindex + numbits - 32); \
		if (bits_in_next_word < 0) \
		{ \
			lcode = (lcode << numbits) | data; \
			bitindex += numbits; \
		} \
		else \
		{ \
			lcode = (lcode << (32 - bitindex)) | (data >> bits_in_next_word); \
			if ((*output_ptr ++ = (UINT8)(lcode >> 24)) == 0xff) \
				*output_ptr ++ = 0; \
			if ((*output_ptr ++ = (UINT8)(lcode >> 16)) == 0xff) \
				*output_ptr ++ = 0; \
			if ((*output_ptr ++ = (UINT8)(lcode >> 8)) == 0xff) \
				*output_ptr ++ = 0; \
			if ((*output_ptr ++ = (UINT8) lcode) == 0xff) \
				*output_ptr ++ = 0; \
			lcode = data; \
			bitindex = bits_in_next_word; \
		} \
	}

static UINT8 *huffman(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure, UINT16 component, UINT8 *output_ptr)
{
	UINT16 i;
	UINT16 *DcCodeTable, *DcSizeTable, *AcCodeTable, *AcSizeTable;

	INT16 *Temp_Ptr, Coeff, LastDc;
	UINT16 AbsCoeff, HuffCode, HuffSize, RunLength = 0, DataSize = 0, index;

	INT16 bits_in_next_word;
	UINT16 numbits;
	UINT32 data;

	Temp_Ptr = Temp;
	Coeff = *Temp_Ptr++;

	if (component == 1)
	{
		DcCodeTable = luminance_dc_code_table;
		DcSizeTable = luminance_dc_size_table;
		AcCodeTable = luminance_ac_code_table;
		AcSizeTable = luminance_ac_size_table;

		LastDc = jpeg_encoder_structure->ldc1;
		jpeg_encoder_structure->ldc1 = Coeff;
	}
	else
	{
		DcCodeTable = chrominance_dc_code_table;
		DcSizeTable = chrominance_dc_size_table;
		AcCodeTable = chrominance_ac_code_table;
		AcSizeTable = chrominance_ac_size_table;

		if (component == 2)
		{
			LastDc = jpeg_encoder_structure->ldc2;
			jpeg_encoder_structure->ldc2 = Coeff;
		}
		else
		{
			LastDc = jpeg_encoder_structure->ldc3;
			jpeg_encoder_structure->ldc3 = Coeff;
		}
	}

	Coeff -= LastDc;

	AbsCoeff = (Coeff < 0) ? -Coeff-- : Coeff;

	while (AbsCoeff != 0)
	{
		AbsCoeff >>= 1;
		DataSize++;
	}

	HuffCode = DcCodeTable[DataSize];
	HuffSize = DcSizeTable[DataSize];


	Coeff &= (1 << DataSize) - 1;
	data = (HuffCode << DataSize) | Coeff;
	numbits = HuffSize + DataSize;

	PUTBITS;

	for (i = 63; i > 0; i--)
	{
		if ((Coeff = *Temp_Ptr++) != 0)
		{
			while (RunLength > 15)
			{
				RunLength -= 16;
				data = AcCodeTable[161];
				numbits = AcSizeTable[161];
				PUTBITS;
			}

			AbsCoeff = (Coeff < 0) ? -Coeff-- : Coeff;

			if (AbsCoeff >> 8 == 0)
			{
				DataSize = bitsize[AbsCoeff];
			}
			else
			{
				DataSize = bitsize[AbsCoeff >> 8] + 8;
			}

			index = RunLength * 10 + DataSize;
			HuffCode = AcCodeTable[index];
			HuffSize = AcSizeTable[index];

			Coeff &= (1 << DataSize) - 1;
			data = (HuffCode << DataSize) | Coeff;
			numbits = HuffSize + DataSize;

			PUTBITS;
			RunLength = 0;
		}
		else
		{
			RunLength++;
		}
	}

	if (RunLength != 0)
	{
		data = AcCodeTable[0];
		numbits = AcSizeTable[0];
		PUTBITS;
	}
	return output_ptr;
}


static UINT8 *encodeMCU(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure, UINT32 image_format, UINT8 *output_ptr)
{
	DCT(Y1);
	quantization(Y1, zigzag_table, Temp, ILqt);
	output_ptr = huffman(jpeg_encoder_structure, 1, output_ptr);

	if ((image_format == JPEG_FORMAT_FOUR_TWO_TWO) || (image_format == JPEG_FORMAT_RGB))
	{
		DCT(Y2);
		quantization(Y2, zigzag_table, Temp, ILqt);
		output_ptr = huffman(jpeg_encoder_structure, 1, output_ptr);

		DCT(CB);
		quantization(CB, zigzag_table, Temp, ICqt);
		output_ptr = huffman(jpeg_encoder_structure, 2, output_ptr);

		DCT(CR);
		quantization(CR, zigzag_table, Temp, ICqt);
		output_ptr = huffman(jpeg_encoder_structure, 3, output_ptr);
	}

	return output_ptr;
}


/* For bit Stuffing and EOI marker */
static UINT8 *close_bitstream(UINT8 *output_ptr)
{
	UINT16 i, count;
	UINT8 *ptr;

	if (bitindex > 0)
	{
		lcode <<= (32 - bitindex);

		count = (bitindex + 7) >> 3;

		ptr = (UINT8 *) &lcode + 3;

		for (i = 0; i < count; i++)
			if ((*output_ptr++ = *ptr--) == 0xff)
			{
				*output_ptr++ = 0;
			}
	}

	// End of image marker
	*output_ptr++ = 0xFF;
	*output_ptr++ = 0xD9;
	return output_ptr;
}


static UINT8 *write_markers(UINT8 *output_ptr, UINT32 image_format, UINT32 image_width, UINT32 image_height)
{
	UINT16 i, header_length;
	UINT8 number_of_components;

	// Start of image marker
	*output_ptr++ = 0xFF;
	*output_ptr++ = 0xD8;

	// Quantization table marker
	*output_ptr++ = 0xFF;
	*output_ptr++ = 0xDB;

	// Quantization table length
	*output_ptr++ = 0x00;
	*output_ptr++ = 0x43;

	// Pq, Tq
	*output_ptr++ = 0x00;

	// Lqt table
	for (i = 0; i < 64; i++)
	{
		*output_ptr++ = Lqt[i];
	}

	// Quantization table marker
	*output_ptr++ = 0xFF;
	*output_ptr++ = 0xDB;

	// Quantization table length
	*output_ptr++ = 0x00;
	*output_ptr++ = 0x43;

	// Pq, Tq
	*output_ptr++ = 0x01;

	// Cqt table
	for (i = 0; i < 64; i++)
	{
		*output_ptr++ = Cqt[i];
	}

	if (image_format == JPEG_FORMAT_FOUR_ZERO_ZERO)
	{
		number_of_components = 1;
	}
	else
	{
		number_of_components = 3;
	}

	// Frame header(SOF)

	// Start of frame marker
	*output_ptr++ = 0xFF;
	*output_ptr++ = 0xC0;

	header_length = (UINT16)(8 + 3 * number_of_components);

	// Frame header length
	*output_ptr++ = (UINT8)(header_length >> 8);
	*output_ptr++ = (UINT8) header_length;

	// Precision (P)
	*output_ptr++ = 0x08;

	// image height
	*output_ptr++ = (UINT8)(image_height >> 8);
	*output_ptr++ = (UINT8) image_height;

	// image width
	*output_ptr++ = (UINT8)(image_width >> 8);
	*output_ptr++ = (UINT8) image_width;

	// Nf
	*output_ptr++ = number_of_components;

	if (image_format == JPEG_FORMAT_FOUR_ZERO_ZERO)
	{
		*output_ptr++ = 0x01;
		*output_ptr++ = 0x11;
		*output_ptr++ = 0x00;
	}
	else
	{
		*output_ptr++ = 0x01;

		if ((image_format == JPEG_FORMAT_FOUR_TWO_TWO) || (image_format == JPEG_FORMAT_RGB))
		{
			*output_ptr++ = 0x21;
		}
		else
		{
			*output_ptr++ = 0x11;
		}

		*output_ptr++ = 0x00;

		*output_ptr++ = 0x02;
		*output_ptr++ = 0x11;
		*output_ptr++ = 0x01;

		*output_ptr++ = 0x03;
		*output_ptr++ = 0x11;
		*output_ptr++ = 0x01;
	}

	// huffman table(DHT)

	for (i = 0; i < sizeof(markerdata); i++)
	{
		*output_ptr++ = markerdata[i];
	}

	// Scan header(SOF)

	// Start of scan marker
	*output_ptr++ = 0xFF;
	*output_ptr++ = 0xDA;

	header_length = (UINT16)(6 + (number_of_components << 1));

	// Scan header length
	*output_ptr++ = (UINT8)(header_length >> 8);
	*output_ptr++ = (UINT8) header_length;

	// Ns
	*output_ptr++ = number_of_components;

	if (image_format == JPEG_FORMAT_FOUR_ZERO_ZERO)
	{
		*output_ptr++ = 0x01;
		*output_ptr++ = 0x00;
	}
	else
	{
		*output_ptr++ = 0x01;
		*output_ptr++ = 0x00;

		*output_ptr++ = 0x02;
		*output_ptr++ = 0x11;

		*output_ptr++ = 0x03;
		*output_ptr++ = 0x11;
	}

	*output_ptr++ = 0x00;
	*output_ptr++ = 0x3F;
	*output_ptr++ = 0x00;
	return output_ptr;
}


/* Multiply Quantization table with quality factor to get LQT and CQT
   factor ranges from 1 to 8;  1 = highest quality,  8 = lowest quality */
static void initialize_quantization_tables(UINT32 quality_factor)
{
	UINT16 i, index;
	UINT32 value;

	if (quality_factor < 1)
	{
		quality_factor = 1;
	}
	if (quality_factor > 8)
	{
		quality_factor = 8;
	}
	quality_factor = ((quality_factor * 3) - 2) * 128; //converts range[1:8] to [1:22]

	for (i = 0; i < 64; i++)
	{
		index = zigzag_table[i];

		/* luminance quantization table * quality factor */
		value = luminance_quant_table[i] * quality_factor;
		value = (value + 0x200) >> 10;

		if (value < 2)
		{
			value = 2;
		}
		else if (value > 255)
		{
			value = 255;
		}

		Lqt[index] = (UINT8) value;
		ILqt[i] = 0x8000 / value;

		/* chrominance quantization table * quality factor */
		value = chrominance_quant_table[i] * quality_factor;
		value = (value + 0x200) >> 10;

		if (value < 2)
		{
			value = 2;
		}
		else if (value > 255)
		{
			value = 255;
		}

		Cqt[index] = (UINT8) value;
		ICqt[i] = 0x8000 / value;
	}
}


UINT8 *jpeg_encode_image(UINT8 *input_ptr, UINT8 *output_ptr, UINT32 quality_factor, UINT32 image_format, UINT32 image_width, UINT32 image_height)
{
	UINT16 i, j;

	JPEG_ENCODER_STRUCTURE JpegStruct;
	JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure = &JpegStruct;

	/* Initialization of JPEG control structure */
	initialization(jpeg_encoder_structure, image_format, image_width, image_height);

	/* Quantization Table Initialization */
	initialize_quantization_tables(quality_factor);

	/* Writing Marker Data */
	output_ptr = write_markers(output_ptr, image_format, image_width, image_height);

	for (i = 1; i <= jpeg_encoder_structure->vertical_mcus; i++)
	{
		if (i < jpeg_encoder_structure->vertical_mcus)
		{
			jpeg_encoder_structure->rows = jpeg_encoder_structure->mcu_height;
		}
		else
		{
			jpeg_encoder_structure->rows = jpeg_encoder_structure->rows_in_bottom_mcus;
		}

		for (j = 1; j <= jpeg_encoder_structure->horizontal_mcus; j++)
		{
			if (j < jpeg_encoder_structure->horizontal_mcus)
			{
				jpeg_encoder_structure->cols = jpeg_encoder_structure->mcu_width;
				jpeg_encoder_structure->incr = jpeg_encoder_structure->length_minus_mcu_width;
			}
			else
			{
				jpeg_encoder_structure->cols = jpeg_encoder_structure->cols_in_right_mcus;
				jpeg_encoder_structure->incr = jpeg_encoder_structure->length_minus_width;
			}

			read_format(jpeg_encoder_structure, input_ptr);

			/* Encode the data in MCU */
			output_ptr = encodeMCU(jpeg_encoder_structure, image_format, output_ptr);

			input_ptr += jpeg_encoder_structure->mcu_width_size;
		}

		input_ptr += jpeg_encoder_structure->offset;
	}

	/* Close Routine */
	output_ptr = close_bitstream(output_ptr);
	return output_ptr;
}

