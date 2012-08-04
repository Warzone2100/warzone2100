/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
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

/** \file
 *  header of the routines used to render characters with textures.
 */

#ifndef __glc_texture_h
#define __glc_texture_h

#include "ofont.h"

#define GLC_TEXTURE_SIZE        64

struct __GLCatlasElementRec {
  FT_ListNodeRec node;

  int position;
  __GLCglyph* glyph;
};

void __glcReleaseAtlasElement(__GLCatlasElement* This, __GLCcontext* inContext);
void __glcRenderCharTexture(const __GLCfont* inFont, __GLCcontext* inContext,
			    const GLfloat inScaleX, const GLfloat inScaleY,
			    __GLCglyph* inGlyph);
#endif
