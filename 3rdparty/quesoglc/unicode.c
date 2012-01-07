/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2009, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$ */

/* This file defines miscellaneous utility routines used for Unicode management
 */

/** \file
 *  defines the routines used to manipulate Unicode strings and characters
 */

#include <fribidi/fribidi.h>

#include "internal.h"



/* Find a Unicode name from its code */
const GLCchar8* __glcNameFromCode(const GLint code)
{
  GLint position = -1;

  if ((code < 0) || (code > __glcMaxCode)) {
    static char buffer[20];

    if (code > 0x10ffff) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return NULL;
    }

    snprintf(buffer, 20, "Character 0x%x", code);
    return (const GLCchar8*)buffer; 
  }

  position = __glcNameFromCodeArray[code];
  if (position == -1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  return (const GLCchar8*)__glcCodeFromNameArray[position].name;
}



/* Find a Unicode code from its name */
GLint __glcCodeFromName(const GLCchar8* name)
{
  int start = 0;
  int end = __glcCodeFromNameSize;
  int middle = (end + start) / 2;
  int res = 0;

  while (end - start > 1) {
    res = strcmp((const char*)name, __glcCodeFromNameArray[middle].name);
    if (res > 0)
      start = middle;
    else if (res < 0)
      end = middle;
    else
      return __glcCodeFromNameArray[middle].code;
    middle = (end + start) / 2;
  }
  if (strcmp((const char*)name, __glcCodeFromNameArray[start].name) == 0)
    return __glcCodeFromNameArray[start].code;
  if (strcmp((const char*)name, __glcCodeFromNameArray[end].name) == 0)
    return __glcCodeFromNameArray[end].code;

  __glcRaiseError(GLC_PARAMETER_ERROR);
  return -1;
}



/* Convert a character from UCS1 to UTF-8 and return the number of bytes
 * needed to encode the char.
 */
static int __glcUcs1ToUtf8(const GLCchar8 ucs1, GLCchar8 dest[FC_UTF8_MAX_LEN])
{
  GLCchar8 *d = dest;

  if (ucs1 < 0x80)
    *d++ = ucs1;
  else {
    *d++ = ((ucs1 >> 6) & 0x1F) | 0xC0;
    *d++ = (ucs1 & 0x3F) | 0x80;
  }

  return d - dest;
}



/* Convert a character from UCS2 to UTF-8 and return the number of bytes
 * needed to encode the char.
 */
static int __glcUcs2ToUtf8(const GLCchar16 ucs2, GLCchar8 dest[FC_UTF8_MAX_LEN])
{
  GLCchar8 *d = dest;

  if (ucs2 < 0x80)
    *d++ = ucs2;
  else if (ucs2 < 0x800) {
    *d++ = ((ucs2 >> 6) & 0x1F) | 0xC0;
    *d++ = (ucs2 & 0x3F) | 0x80;
  }
  else {
    *d++ = ((ucs2 >> 12) & 0x0F) | 0xE0;
    *d++ = ((ucs2 >> 6) & 0x3F) | 0x80;
    *d++ = (ucs2 & 0x3F) | 0x80;
  }

  return d - dest;
}



/* Convert a character from UTF-8 to UCS1 and return the number of bytes
 * needed to encode the character.
 * According to the GLC specs, when the value of a character code exceed the
 * range of the character encoding, the returned character is converted
 * to a character sequence \<hexcode> where 'hexcode' is the original
 * character code represented as a sequence of hexadecimal digits
 */
static int __glcUtf8ToUcs1(const GLCchar8* src_orig,
			   GLCchar8 dst[GLC_OUT_OF_RANGE_LEN], int len,
			   int* dstlen)
{
  GLCchar32 result = 0;
  int src_shift = FcUtf8ToUcs4(src_orig, &result, len);

  if (src_shift > 0) {
    /* src_orig is a well-formed UTF-8 character */
    if (result < 0x100) {
      *dst = result;
      *dstlen = 1;
    }
    else {
      /* Convert to the string '\<xxx>' */
#ifdef _MSC_VER
      *dstlen = sprintf_s((char*)dst, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* sprintf_s returns -1 on any error, and the number of characters
       * written to the string not including the terminating null otherwise.
       * Insufficient length of the destination buffer is an error and the
       * buffer is set to an empty string. */
      if (*dstlen < 0)
        *dstlen = 0;
#else
      *dstlen = snprintf((char*)dst, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* Standard ISO/IEC 9899:1999 (ISO C99) snprintf, which it appears
       * Microsoft has not implemented for their operating systems. Return
       * value is length of the string that would have been written into
       * the destination buffer not including the terminating null had their
       * been enough space. Truncation has occurred if return value is >= 
       * destination buffer size. */
      if (*dstlen >= GLC_OUT_OF_RANGE_LEN)
        *dstlen = GLC_OUT_OF_RANGE_LEN - 1;
#endif
    }
  }
  return src_shift;
}



/* Convert a character from UTF-8 to UCS1 and return the number of bytes
 * needed to encode the character.
 * According to the GLC specs, when the value of a character code exceed the
 * range of the character encoding, the returned character is converted
 * to a character sequence \<hexcode> where 'hexcode' is the original
 * character code represented as a sequence of hexadecimal digits
 */
static int __glcUtf8ToUcs2(const GLCchar8* src_orig,
			   GLCchar16 dst[GLC_OUT_OF_RANGE_LEN], int len,
			   int* dstlen)
{
  GLCchar32 result = 0;
  int src_shift = FcUtf8ToUcs4(src_orig, &result, len);

  if (src_shift > 0) {
    /* src_orig is a well-formed UTF-8 character */
    if (result < 0x10000) {
      *dst = result;
      *dstlen = 1;
    }
    else {
      /* Convert to the string '\<xxx>' */
      int count;
      char* src = NULL;
      char buffer[GLC_OUT_OF_RANGE_LEN];

#ifdef _MSC_VER
      *dstlen = sprintf_s((char*)buffer, GLC_OUT_OF_RANGE_LEN, "\\<%X>",
			  result);
      /* sprintf_s returns -1 on any error, and the number of characters
       * written to the string not including the terminating null otherwise.
       * Insufficient length of the destination buffer is an error and the
       * buffer is set to an empty string. */
      if (*dstlen < 0)
        *dstlen = 0;
#else
      *dstlen = snprintf((char*)buffer, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* Standard ISO/IEC 9899:1999 (ISO C99) snprintf, which it appears
       * Microsoft has not implemented for their operating systems. Return
       * value is length of the string that would have been written into
       * the destination buffer not including the terminating null had their
       * been enough space. Truncation has occurred if return value is >= 
       * destination buffer size. */
      if (*dstlen >= GLC_OUT_OF_RANGE_LEN)
        *dstlen = GLC_OUT_OF_RANGE_LEN - 1;
#endif
      for (count = 0, src = buffer; count < *dstlen; count++, *dst++ = *src++);
      *dst = 0; /* Terminating '\0' character */
    }
  }
  return src_shift;
}



/* Convert 'inString' in the UTF-8 format and return a copy of the converted
 * string.
 */
GLCchar8* __glcConvertToUtf8(const GLCchar* inString, const GLint inStringType)
{
  GLCchar8 buffer[FC_UTF8_MAX_LEN > 8 ? FC_UTF8_MAX_LEN : 8];
  GLCchar8* string = NULL;
  GLCchar8* ptr = NULL;
  int len;

  switch(inStringType) {
  case GLC_UCS1:
    {
      const GLCchar8* ucs1 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs1 = (const GLCchar8*)inString; *ucs1;
	     len += __glcUcs1ToUtf8(*ucs1++, buffer));
      /* Allocate the room to store the final string */
      string = (GLCchar8*)__glcMalloc((len+1)*sizeof(GLCchar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs1 = (const GLCchar8*)inString, ptr = string; *ucs1;
	     ptr += __glcUcs1ToUtf8(*ucs1++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UCS2:
    {
      const GLCchar16* ucs2 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs2 = (const GLCchar16*)inString; *ucs2;
	     len += __glcUcs2ToUtf8(*ucs2++, buffer));
      /* Allocate the room to store the final string */
      string = (GLCchar8*)__glcMalloc((len+1)*sizeof(GLCchar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs2 = (const GLCchar16*)inString, ptr = string; *ucs2;
	   ptr += __glcUcs2ToUtf8(*ucs2++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UCS4:
    {
      const GLCchar32* ucs4 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs4 = (const GLCchar32*)inString; *ucs4;
	     len += FcUcs4ToUtf8(*ucs4++, buffer));
      /* Allocate the room to store the final string */
      string = (GLCchar8*)__glcMalloc((len+1)*sizeof(GLCchar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs4 = (const GLCchar32*)inString, ptr = string; *ucs4;
	   ptr += FcUcs4ToUtf8(*ucs4++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UTF8_QSO:
    /* If the string is already encoded in UTF-8 format then all we need to do
     * is to make a copy of it.
     */
#ifdef __WIN32__
    string = (GLCchar8*)_strdup((const char*)inString);
#else
    string = (GLCchar8*)strdup((const char*)inString);
#endif
    break;
  default:
    return NULL;
  }

  return string;
}



/* Convert 'inString' from the UTF-8 format and return a copy of the
 * converted string in the context buffer.
 */
GLCchar* __glcConvertFromUtf8ToBuffer(__GLCcontext* This,
				      const GLCchar8* inString)
{
  GLCchar* string = NULL;
  const GLCchar8* utf8 = NULL;
  int len_buffer = 0;
  int len = 0;
  int shift = 0;

  assert(inString);

  switch(This->stringState.stringType) {
  case GLC_UCS1:
    {
      GLCchar8 buffer[GLC_OUT_OF_RANGE_LEN];
      GLCchar8* ucs1 = NULL;

      /* Determine the length of the final string */
      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs1(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcContextQueryBuffer(This,
						 (len+1)*sizeof(GLCchar8));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      ucs1 = (GLCchar8*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs1(utf8, ucs1, strlen((const char*)utf8),
				&len_buffer);
	ucs1 += len_buffer;
      }

      *ucs1 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      GLCchar16 buffer[GLC_OUT_OF_RANGE_LEN];
      GLCchar16* ucs2 = NULL;

      /* Determine the length of the final string */
      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs2(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcContextQueryBuffer(This,
						 (len+1)*sizeof(GLCchar16));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      ucs2 = (GLCchar16*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs2(utf8, ucs2, strlen((const char*)utf8),
				&len_buffer);
	ucs2 += len_buffer;
      }
      *ucs2 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      GLCchar32 buffer = 0;
      GLCchar32* ucs4 = NULL;

      /* Determine the length of the final string */
      utf8 = inString;
      while(*utf8) {
	shift = FcUtf8ToUcs4(utf8, &buffer, strlen((const char*)utf8));
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len++;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcContextQueryBuffer(This,
						 (len+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */


      /* Perform the conversion */
      utf8 = inString;
      ucs4 = (GLCchar32*)string;
      while(*utf8)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    /* If the string is already encoded in UTF-8 format then all we need to do
     * is to make a copy of it.
     */
    string = (GLCchar*)__glcContextQueryBuffer(This,
				 	       strlen((const char*)inString)+1);
    if (!string)
      return NULL; /* GLC_RESOURCE_ERROR has been raised */
    strcpy((char*)string, (const char*)inString);
    break;
  default:
    return NULL;
  }
  return string;
}



/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. If the code can not be converted to the current string
 * type a GLC_PARAMETER_ERROR is issued.
 */
GLint __glcConvertUcs4ToGLint(__GLCcontext *inContext, GLint inCode)
{
  switch(inContext->stringState.stringType) {
  case GLC_UCS2:
    /* Check that inCode can be stored in UCS-2 format */
    if (inCode <= 65535)
      break;
  case GLC_UCS1:
    /* Check that inCode can be stored in UCS-1 format */
    if (inCode <= 255)
      break;
  case GLC_UTF8_QSO:
    /* A Unicode codepoint can be no higher than 0x10ffff
     * (see Unicode specs)
     */
    if (inCode > 0x10ffff) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
    else {
      /* Codepoints lower or equal to 0x10ffff can be encoded on 4 bytes in
       * UTF-8 format
       */
      GLCchar8 buffer[FC_UTF8_MAX_LEN > 8 ? FC_UTF8_MAX_LEN : 8];
#ifndef NDEBUG
      int len = FcUcs4ToUtf8((GLCchar32)inCode, buffer);
      assert((size_t)len <= sizeof(GLint));
#else
      FcUcs4ToUtf8((GLCchar32)inCode, buffer);
#endif

      return *((GLint*)buffer);
    }
  }

  return inCode;
}



/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character codes
 * in GLint which may cause problems for the UTF-8 format.
 */
GLint __glcConvertGLintToUcs4(const __GLCcontext *inContext, GLint inCode)
{
  GLint code = inCode;

  if (inCode < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return -1;
  }

  switch (inContext->stringState.stringType) {
  case GLC_UCS1:
    if (inCode > 0xff) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
    break;
  case GLC_UCS2:
    if (inCode > 0xffff) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
    break;
  case GLC_UTF8_QSO:
    /* Convert the codepoint in UCS4 format and check if it is ill-formed or
     * not
     */
    if (FcUtf8ToUcs4((GLCchar8*)&inCode, (GLCchar32*)&code,
		     sizeof(GLint)) < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
    break;
  }

  return code;
}



/* Convert 'inString' (stored in logical order) to UCS4 format and return a
 * copy of the converted string in visual order.
 */
GLCchar32* __glcConvertToVisualUcs4(__GLCcontext* inContext,
				    GLboolean *outIsRTL, GLint *outLength,
				    const GLCchar* inString)
{
  GLCchar32* string = NULL;
  int length = 0;
  FriBidiCharType base = FRIBIDI_TYPE_ON;
  GLCchar32* visualString = NULL;

  assert(inString);

  switch(inContext->stringState.stringType) {
  case GLC_UCS1:
    {
      const GLCchar8* ucs1 = (const GLCchar8*)inString;
      GLCchar32* ucs4 = NULL;

      length = strlen((const char*)ucs1);

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      for (ucs4 = string; *ucs1; ucs1++, ucs4++)
	*ucs4 = (GLCchar32)(*ucs1);

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      const GLCchar16* ucs2 = NULL;
      GLCchar32* ucs4 = NULL;

      for (ucs2 = (const GLCchar16*)inString; *ucs2; ucs2++, length++);

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      for (ucs2 = (const GLCchar16*)inString, ucs4 = string; *ucs2;
	   ucs2++, ucs4++)
	*ucs4 = (GLCchar32)(*ucs2);

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      const GLCchar32* ucs4 = NULL;

      for (ucs4 = (const GLCchar32*)inString; *ucs4; ucs4++, length++);

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(int));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      memcpy(string, inString, length*sizeof(int));

      ((int*)string)[length] = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    {
      GLCchar32* ucs4 = NULL;
      const GLCchar8* utf8 = NULL;
      GLCchar32 buffer = 0;
      int shift = 0;

      /* Determine the length of the final string */
      utf8 = (const GLCchar8*)inString;
      while(*utf8) {
	shift = FcUtf8ToUcs4(utf8, &buffer, strlen((const char*)utf8));
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  return NULL;
	}
	utf8 += shift;
	length++;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      utf8 = (const GLCchar8*)inString;
      ucs4 = (GLCchar32*)string;
      while(*utf8)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  }

  if (length) {
    visualString = string + length + 1;
    if (!fribidi_log2vis(string, length, &base, visualString, NULL, NULL,
                         NULL)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    *outIsRTL = FRIBIDI_IS_RTL(base) ? GL_TRUE : GL_FALSE;
  }
  else
    visualString = string;

  *outLength = length;

  return visualString;
}



/* Convert 'inCount' characters of 'inString' (stored in logical order) to UCS4
 * format and return a copy of the converted string in visual order.
 */
GLCchar32* __glcConvertCountedStringToVisualUcs4(__GLCcontext* inContext,
						GLboolean *outIsRTL,
						const GLCchar* inString,
						const GLint inCount)
{
  GLCchar32* string = NULL;
  FriBidiCharType base = FRIBIDI_TYPE_ON;
  GLCchar32* visualString = NULL;

  assert(inString);

  switch(inContext->stringState.stringType) {
  case GLC_UCS1:
    {
      const GLCchar8* ucs1 = (const GLCchar8*)inString;
      GLCchar32* ucs4 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      ucs4 = string;

      for (i = 0; i < inCount; i++)
	*(ucs4++) = (GLCchar32)(*(ucs1++));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      const GLCchar16* ucs2 = NULL;
      GLCchar32* ucs4 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      ucs2 = (const GLCchar16*)inString;
      ucs4 = string;

      for (i = 0 ; i < inCount; i++)
	*(ucs4++) = (GLCchar32)(*(ucs2++));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(int));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      memcpy(string, inString, inCount*sizeof(int));

      ((int*)string)[inCount] = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    {
      GLCchar32* ucs4 = NULL;
      const GLCchar8* utf8 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      utf8 = (const GLCchar8*)inString;
      ucs4 = (GLCchar32*)string;

      for (i = 0; i < inCount; i++)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  }

  visualString = string + inCount;
  if (!fribidi_log2vis(string, inCount, &base, visualString, NULL, NULL,
		       NULL)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  *outIsRTL = FRIBIDI_IS_RTL(base) ? GL_TRUE : GL_FALSE;

  return visualString;
}
