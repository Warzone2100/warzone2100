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

/** \file
 * defines the object __GLCcontext which is used to manage the contexts.
 */

#if defined(__WIN32__) || defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "internal.h"
#include "texture.h"
#include FT_MODULE_H

__GLCcommonArea __glcCommonArea;
#ifdef HAVE_TLS
__thread __GLCthreadArea __glcTlsThreadArea;
#else
__GLCthreadArea* __glcThreadArea = NULL;
#endif

static GLboolean __glcContextUpdateHashTable(__GLCcontext *This);



/* Find a token in a list of tokens separated by 'separator' */
static GLCchar8* __glcFindIndexList(GLCchar8* inString, const GLuint inIndex,
				    const GLCchar8* inSeparator)
{
  GLuint occurence = 0;
  GLCchar8* s = inString;
  const GLCchar8* sep = inSeparator;

  if (!inIndex)
    return s;

  for (; *s != '\0'; s++) {
    if (*s == *sep) {
      occurence++;
      if (occurence == inIndex)
	break;
    }
  }

  return (GLCchar8 *) s;
}



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 */
__GLCcontext* __glcContextCreate(const GLint inContext)
{
  __GLCcontext *This = NULL;

  This = (__GLCcontext*)__glcMalloc(sizeof(__GLCcontext));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCcontext));

  if (FT_New_Library(&__glcCommonArea.memoryManager, &This->library)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  FT_Add_Default_Modules(This->library);

#ifdef GLC_FT_CACHE
  if (FTC_Manager_New(This->library, 0, 0, 0, __glcFileOpen, NULL,
		      &This->cache)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
#endif

  __glcLock();
  This->config = FcInitLoadConfigAndFonts();
  __glcUnlock();
  if (!This->config) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }

  This->catalogList = __glcArrayCreate(sizeof(GLCchar8*));
  if (!This->catalogList) {
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->masterHashTable = __glcArrayCreate(sizeof(GLCchar32));
  if (!This->masterHashTable) {
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  if (!__glcContextUpdateHashTable(This)) {
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->currentFontList.head = NULL;
  This->currentFontList.tail = NULL;
  This->fontList.head = NULL;
  This->fontList.tail = NULL;
  This->genFontList.head = NULL;
  This->genFontList.tail = NULL;

  This->isCurrent = GL_FALSE;
  This->isInGlobalCommand = GL_FALSE;
  This->id = inContext;
  This->pendingDelete = GL_FALSE;
  This->stringState.callback = (GLCfunc)GLC_NONE;
  This->stringState.dataPointer = (void*)GLC_NONE;
  This->stringState.stringType = GLC_UCS1;
  This->enableState.autoFont = GL_TRUE;
  This->enableState.glObjects = GL_TRUE;
  This->enableState.mipmap = GL_TRUE;
  This->enableState.hinting = GL_FALSE;
  This->enableState.extrude = GL_FALSE;
  This->enableState.kerning = GL_FALSE;
  This->renderState.resolution = 72.;
  This->renderState.renderStyle = GLC_BITMAP;
  This->renderState.tolerance = 0.005;
  This->bitmapMatrixStackDepth = 1;
  This->bitmapMatrix = This->bitmapMatrixStack;
  This->bitmapMatrix[0] = 1.;
  This->bitmapMatrix[3] = 1.;
  This->measurementBuffer = __glcArrayCreate(12 * sizeof(GLfloat));
  if (!This->measurementBuffer) {
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  This->isInCallbackFunc = GL_FALSE;
  This->vertexArray = __glcArrayCreate(2 * sizeof(GLfloat));
  if (!This->vertexArray) {
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  This->controlPoints = __glcArrayCreate(5 * sizeof(GLfloat));
  if (!This->controlPoints) {
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  This->endContour = __glcArrayCreate(sizeof(int));
  if (!This->endContour) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->vertexIndices = __glcArrayCreate(sizeof(GLuint));
  if (!This->vertexIndices) {
    __glcArrayDestroy(This->endContour);
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->geomBatches = __glcArrayCreate(sizeof(__GLCgeomBatch));
  if (!This->geomBatches) {
    __glcArrayDestroy(This->vertexIndices);
    __glcArrayDestroy(This->endContour);
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  /* The environment variable GLC_PATH is an alternate way to allow QuesoGLC
   * to access to fonts catalogs/directories.
   */
  /*Check if the GLC_PATH environment variables are exported */
  if (getenv("GLC_CATALOG_LIST") || getenv("GLC_PATH")) {
    GLCchar8 *path = NULL;
    GLCchar8 *begin = NULL;
    GLCchar8 *sepPos = NULL;
    const GLCchar8 *separator = (GLCchar8*)getenv("GLC_LIST_SEPARATOR");

    /* Get the list separator */
    if (!separator) {
#ifdef __WIN32__
      /* Windows can not use a colon-separated list since the colon sign is
       * used after the drive letter. The semicolon is used by Windows for its
       * PATH variable, so we use it for consistency.
       */
      separator = (const GLCchar8*)";";
#else
      /* POSIX platforms use colon-separated lists for the paths variables
       * so we keep with it for consistency.
       */
      separator = (const GLCchar8*)":";
#endif
    }

    /* Read the paths of fonts file.
     * First, try GLC_CATALOG_LIST...
     */
    if (getenv("GLC_CATALOG_LIST"))
#ifdef __WIN32__
      path = (GLCchar8*)_strdup(getenv("GLC_CATALOG_LIST"));
#else
      path = (GLCchar8*)strdup(getenv("GLC_CATALOG_LIST"));
#endif
    /* Then try GLC_PATH */
    else if (getenv("GLC_PATH")) {
#ifdef __WIN32__
      path = (GLCchar8*)_strdup(getenv("GLC_PATH"));
#else
      path = (GLCchar8*)strdup(getenv("GLC_PATH"));
#endif
    }

    if (path) {
      /* Get each path and add the corresponding masters to the current
       * context */
      GLCchar8* duplicated = NULL;

      begin = path;
      do {
	sepPos = __glcFindIndexList(begin, 1, separator);

        if (*sepPos)
	  *(sepPos++) = 0;

#ifdef __WIN32__
        duplicated = (GLCchar8*)_strdup((char*)begin);
#else
        duplicated = (GLCchar8*)strdup((char*)begin);
#endif
        if (!duplicated) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	}
        else {
	  if (!__glcArrayAppend(This->catalogList, &duplicated))
            free(duplicated);
          else if (!FcConfigAppFontAddDir(This->config,
                                          (const unsigned char*)begin)) {
            __glcArrayRemove(This->catalogList,
                             GLC_ARRAY_LENGTH(This->catalogList));
	    __glcRaiseError(GLC_RESOURCE_ERROR);
            free(duplicated);
          }
	  else if (!__glcContextUpdateHashTable(This)) {
	    /* For some reason the update of the master hash table has failed :
	     * the new catalog must then be removed from GLC_CATALOG_LIST.
	     */
	    __glcContextRemoveCatalog(This,
				      GLC_ARRAY_LENGTH(This->catalogList));
	  }
        }

        begin = sepPos;
      } while (*sepPos);
      free(path);
    }
    else {
      /* strdup has failed to allocate memory to duplicate GLC_PATH => ERROR */
      __glcRaiseError(GLC_RESOURCE_ERROR);
    }
  }

  return This;
}



#ifndef GLC_FT_CACHE
/* This function is called from FT_List_Finalize() to close all the fonts
 * of the GLC_CURRENT_FONT_LIST
 */
static void __glcFontClosure(FT_Memory GLC_UNUSED_ARG(inMemory), void* inData,
			     void* GLC_UNUSED_ARG(inUser))
{
  __GLCfont *font = (__GLCfont*)inData;

  assert(font);
  __glcFontClose(font);
}
#endif



/* This function is called from FT_List_Finalize() to destroy all
 * remaining fonts
 */
static void __glcFontDestructor(FT_Memory GLC_UNUSED_ARG(inMemory),
                                void* inData, void* inUser)
{
  __GLCfont *font = (__GLCfont*)inData;
  __GLCcontext* ctx = (__GLCcontext*)inUser;

  assert(ctx);
  assert(font);
  __glcFontDestroy(font, ctx);
}



/* Destructor of the object : it first destroys all the GLC objects that have
 * been created during the life of the context. Then it releases the memory
 * occupied by the GLC state struct.
 * It does not destroy GL objects associated with the GLC context since we can
 * not be sure that the current GL context is the GL context that contains the
 * GL objects built by the GLC context that we are destroying. This could
 * happen if the user calls glcDeleteContext() after the GL context has been
 * destroyed or after the user has changed the current GL context.
 */
void __glcContextDestroy(__GLCcontext *This)
{
  int i = 0;

  assert(This);

  /* Destroy the list of catalogs */
  for (i = 0; i < GLC_ARRAY_LENGTH(This->catalogList); i++) {
    GLCchar8* string = ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[i];

    assert(string);
    free(string);
  }
  __glcArrayDestroy(This->catalogList);

  /* Destroy GLC_CURRENT_FONT_LIST */
#ifdef GLC_FT_CACHE
  FT_List_Finalize(&This->currentFontList, NULL,
		   &__glcCommonArea.memoryManager, NULL);
#else
  FT_List_Finalize(&This->currentFontList, __glcFontClosure,
		   &__glcCommonArea.memoryManager, NULL);
#endif

  /* Destroy GLC_FONT_LIST */
  FT_List_Finalize(&This->fontList, __glcFontDestructor,
                   &__glcCommonArea.memoryManager, This);
  /* Destroy empty fonts generated by glcGenFontID() */
  FT_List_Finalize(&This->genFontList, __glcFontDestructor,
		   &__glcCommonArea.memoryManager, This);

  if (This->masterHashTable)
    __glcArrayDestroy(This->masterHashTable);

  FT_List_Finalize(&This->atlasList, NULL,
		   &__glcCommonArea.memoryManager, NULL);

  if (This->bufferSize)
    __glcFree(This->buffer);

  if (This->measurementBuffer)
    __glcArrayDestroy(This->measurementBuffer);

  if (This->vertexArray)
    __glcArrayDestroy(This->vertexArray);

  if (This->controlPoints)
    __glcArrayDestroy(This->controlPoints);

  if (This->endContour)
    __glcArrayDestroy(This->endContour);

  if (This->vertexIndices)
    __glcArrayDestroy(This->vertexIndices);

  if (This->geomBatches)
    __glcArrayDestroy(This->geomBatches);

#ifdef GLC_FT_CACHE
  FTC_Manager_Done(This->cache);
#endif
  FT_Done_Library(This->library);
  FcConfigDestroy(This->config);
  __glcFree(This);
}



/* Return the first font in GLC_CURRENT_FONT_LIST that maps 'inCode'.
 * If there is no such font, the function returns NULL.
 * 'inCode' must be given in UCS-4 format.
 */
static __GLCfont* __glcLookupFont(const FT_List fontList, const GLint inCode)
{
  FT_ListNode node = NULL;

  for (node = fontList->head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)node->data;

    /* Check if the character identified by inCode exists in the font */
    if (__glcFontHasChar(font, inCode))
      return font;
  }
  return NULL;
}



/* Calls the callback function (does various tests to determine if it is
 * possible) and returns GL_TRUE if it has succeeded or GL_FALSE otherwise.
 * 'inCode' must be given in UCS-4 format.
 */
static GLboolean __glcCallCallbackFunc(__GLCcontext *inContext,
				       const GLint inCode)
{
  GLCfunc callbackFunc = NULL;
  GLboolean result = GL_FALSE;
  GLint aCode = 0;

  /* Recursivity is not allowed */
  if (inContext->isInCallbackFunc)
    return GL_FALSE;

  callbackFunc = inContext->stringState.callback;
  if (!callbackFunc)
    return GL_FALSE;

  /* Convert the character code back to the current string type */
  aCode = __glcConvertUcs4ToGLint(inContext, inCode);
  /* Check if the character has been converted */
  if (aCode < 0)
    return GL_FALSE;

  inContext->isInCallbackFunc = GL_TRUE;
  /* Call the callback function with the character converted to the current
   * string type.
   */
  result = (*callbackFunc)(aCode);
  inContext->isInCallbackFunc = GL_FALSE;

  return result;
}



/* Returns the ID of the first font in GLC_CURRENT_FONT_LIST that maps
 * 'inCode'. If there is no such font and GLC_AUTO_FONT is enabled, the
 * function attempts to append a new font from GLC_FONT_LIST (or from a master)
 * to GLC_CURRENT_FONT_LIST. If the attempt fails the function returns zero.
 * 'inCode' must be given in UCS-4 format.
 */
__GLCfont* __glcContextGetFont(__GLCcontext *This, const GLint inCode)
{
  __GLCfont* font = NULL;

  /* Look for a font in the current font list */
  font = __glcLookupFont(&This->currentFontList, inCode);
  /* If a font has been found return */
  if (font)
    return font;

  /* If a callback function is defined for GLC_OP_glcUnmappedCode then call it.
   * The callback function should return GL_TRUE if it succeeds in appending to
   * GLC_CURRENT_FONT_LIST the ID of a font that maps 'inCode'.
   */
  if (__glcCallCallbackFunc(This, inCode)) {
    font = __glcLookupFont(&This->currentFontList, inCode);
    if (font)
      return font;
  }

  /* If the value of the boolean variable GLC_AUTOFONT is GL_TRUE then search
   * GLC_FONT_LIST for the first font that maps 'inCode'. If the search
   * succeeds, then append the font's ID to GLC_CURRENT_FONT_LIST.
   */
  if (This->enableState.autoFont) {
    __GLCmaster* master = NULL;

    font = __glcLookupFont(&This->fontList, inCode);
    if (font) {
      __glcAppendFont(This, font);
      return font;
    }

    master = __glcMasterMatchCode(This, inCode);
    if (!master)
      return NULL;
    font = __glcNewFontFromMaster(__glcGenFontID(This), master, This, inCode);
    __glcMasterDestroy(master);

    if (font) {
      /* Add the font to the GLC_CURRENT_FONT_LIST */
      __glcAppendFont(This, font);
      return font;
    }
  }
  return NULL;
}



/* Sometimes informations may need to be stored temporarily by a thread.
 * The so-called 'buffer' is created for that purpose. Notice that it is a
 * component of the GLC state struct hence its lifetime is the same as the
 * GLC state's lifetime.
 * __glcContextQueryBuffer() should be called whenever the buffer is to be used
 * The function checks that the buffer is big enough to store the required data
 * and returns a pointer to the buffer.
 * Note that the only memory management function used below is 'realloc' which
 * means that the buffer goes bigger and bigger until it is freed. No function
 * is provided to reduce its size so it should be freed and re-allocated
 * manually in case of emergency ;-)
 */
GLCchar* __glcContextQueryBuffer(__GLCcontext *This, const size_t inSize)
{
  GLCchar* buffer = This->buffer;

  if (inSize > This->bufferSize) {
    buffer = (GLCchar*)__glcRealloc(This->buffer, inSize);
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
    }
    else {
      This->buffer = buffer;
      This->bufferSize = inSize;
    }
  }

  return buffer;
}



/* Update the hash table that which is used to convert master IDs into
 * FontConfig patterns.
 */
static GLboolean __glcContextUpdateHashTable(__GLCcontext *This)
{
  FcPattern* pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  int i = 0;
  __GLCarray *updatedHashTable = NULL;

  /* Use Fontconfig to get the default font files */
  pattern = FcPatternCreate();
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GL_FALSE;
  }
  fontSet = FcFontList(This->config, pattern, objectSet);
  FcPatternDestroy(pattern);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  updatedHashTable = __glcArrayDuplicate(This->masterHashTable);
  if (!updatedHashTable) {
    FcFontSetDestroy(fontSet);
    return GL_FALSE;
  }

  /* Parse the font set looking for fonts that are not already registered in the
   * hash table.
   */
  for (i = 0; i < fontSet->nfont; i++) {
    GLCchar32 hashValue = 0;
    int j = 0;
    const int length = GLC_ARRAY_LENGTH(updatedHashTable);
    GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(updatedHashTable);
    FcBool outline = FcFalse;
    FcChar8* family = NULL;
    int fixed = 0;
    FcChar8* foundry = NULL;
#ifdef DEBUGMODE
    FcResult result = FcResultMatch;

    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
#endif

    /* Check whether the glyphs are outlines */
    if (!outline)
      continue;

#ifdef DEBUGMODE
    result = FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &family);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &foundry);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &family);
    FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &foundry);
    FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed);
#endif

    if (foundry)
      pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family,
			       FC_FOUNDRY, FcTypeString, foundry, FC_SPACING,
			       FcTypeInteger, fixed, NULL);
    else
      pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family,
			       FC_SPACING, FcTypeInteger, fixed, NULL);

    if (!pattern) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcFontSetDestroy(fontSet);
      __glcArrayDestroy(updatedHashTable);
      return GL_FALSE;
    }

    /* Check if the font is already registered in the hash table */
    hashValue = FcPatternHash(pattern);
    FcPatternDestroy(pattern);
    for (j = 0; j < length; j++) {
      if (hashTable[j] == hashValue)
	break;
    }

    /* If the font is already registered then parse the next one */
    if (j != length)
      continue;

    /* Register the font (i.e. append its hash value to the hash table) */
    if (!__glcArrayAppend(updatedHashTable, &hashValue)) {
      FcFontSetDestroy(fontSet);
      __glcArrayDestroy(updatedHashTable);
      return GL_FALSE;
    }
  }

  FcFontSetDestroy(fontSet);
  __glcArrayDestroy(This->masterHashTable);
  This->masterHashTable = updatedHashTable;

  return GL_TRUE;
}



/* Append a catalog to the context catalog list */
void __glcContextAppendCatalog(__GLCcontext* This, const GLCchar* inCatalog)
{
#ifdef __WIN32__
  GLCchar8* duplicated = (GLCchar8*)_strdup((const char*)inCatalog);
#else
  GLCchar8* duplicated = (GLCchar8*)strdup((const char*)inCatalog);
#endif

  if (!duplicated) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (!__glcArrayAppend(This->catalogList, &duplicated)) {
    free(duplicated);
    return;
  }

  if (!FcConfigAppFontAddDir(This->config, (const unsigned char*)inCatalog)) {
    __glcArrayRemove(This->catalogList, GLC_ARRAY_LENGTH(This->catalogList));
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(duplicated);
    return;
  }

  if (!__glcContextUpdateHashTable(This)) {
    /* For some reason the update of the master hash table has failed : the
     * new catalog must then be removed from GLC_CATALOG_LIST.
     */
    __glcContextRemoveCatalog(This, GLC_ARRAY_LENGTH(This->catalogList));
    return;
  }
}



/* Prepend a catalog to the context catalog list */
void __glcContextPrependCatalog(__GLCcontext* This, const GLCchar* inCatalog)
{
#ifdef __WIN32__
  GLCchar8* duplicated = (GLCchar8*)_strdup((const char*)inCatalog);
#else
  GLCchar8* duplicated = (GLCchar8*)strdup((const char*)inCatalog);
#endif

  if (!duplicated) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (!__glcArrayInsert(This->catalogList, 0, &duplicated)) {
    free(duplicated);
    return;
  }

  if (!FcConfigAppFontAddDir(This->config, (const unsigned char*)inCatalog)) {
    __glcArrayRemove(This->catalogList, 0);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(duplicated);
    return;
  }

  if (!__glcContextUpdateHashTable(This)) {
    /* For some reason the update of the master hash table has failed : the
     * new catalog must then be removed from GLC_CATALOG_LIST.
     */
    __glcContextRemoveCatalog(This, 0);
    return;
  }
}



/* Get the path of the catalog identified by inIndex */
GLCchar8* __glcContextGetCatalogPath(const __GLCcontext* This,
				     const GLint inIndex)
{
  if (inIndex >= GLC_ARRAY_LENGTH(This->catalogList)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }
    
  return ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[inIndex];
}



/* This function must be called each time that a command destroys
 * a font. __glcContextDeleteFont removes, if necessary, the font identified by
 * inFont from the list GLC_CURRENT_FONT_LIST and then delete the font.
 */
void __glcContextDeleteFont(__GLCcontext* inContext, __GLCfont* font)
{
  FT_ListNode node = NULL;

  /* Look for the font into GLC_CURRENT_FONT_LIST */
  node = FT_List_Find(&inContext->currentFontList, font);

  /* If the font has been found, remove it from the list */
  if (node) {
    FT_List_Remove(&inContext->currentFontList, node);
#ifndef GLC_FT_CACHE
    __glcFontClose(font);
#endif
    __glcFree(node);
  }
  __glcFontDestroy(font, inContext);
}



/* Remove a catalog from the context catalog list */
void __glcContextRemoveCatalog(__GLCcontext* This, const GLint inIndex)
{
  FT_ListNode node = NULL;
  GLCchar8* catalog = NULL;
  int i = 0;

  if (inIndex >= GLC_ARRAY_LENGTH(This->catalogList)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  FcConfigAppFontClear(This->config);
  catalog = ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[inIndex];
  assert(catalog);
  __glcArrayRemove(This->catalogList, inIndex);
  free(catalog);

  for (i = 0; i < GLC_ARRAY_LENGTH(This->catalogList); i++) {
    catalog = ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[i];
    assert(catalog);
    if (!FcConfigAppFontAddDir(This->config, catalog)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcArrayRemove(This->catalogList, i);
      free(catalog);
      /* After the removal of the problematic catalog , indices are shifted by 1
       */
      if (i > 0) i--;
    }
  }

  /* Re-create the hash table from scratch */
  GLC_ARRAY_LENGTH(This->masterHashTable) = 0;
  __glcContextUpdateHashTable(This);

  /* Remove from GLC_FONT_LIST the fonts that were defined in the catalog that
   * has been removed. This task has to be done even if the call to
   * __glcContextUpdateHashTable() has failed !!!
   */
  for (node = This->fontList.head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)(node->data);
    GLCchar32 hashValue = 0;
    GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(This->masterHashTable);
    const int length = GLC_ARRAY_LENGTH(This->masterHashTable);
    __GLCmaster* master = __glcMasterCreate(font->parentMasterID, This);

    if (!master)
      continue;

    /* Check if the hash value of the master is in the hash table */
    hashValue = GLC_MASTER_HASH_VALUE(master);
    for (i = 0; i < length; i++) {
      if (hashValue == hashTable[i])
	break;
    }

    /* The font is not contained in the hash table => remove it */
    if (i == length) {
      FT_List_Remove(&This->fontList, node);
      __glcContextDeleteFont(This, font);
    }

    __glcMasterDestroy(master);
  }
}
