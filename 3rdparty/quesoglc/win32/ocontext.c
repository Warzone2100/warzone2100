/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2007, Bertrand Coconnier
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

/* QuesoGLC needs Windows 2000 or newer */
#define _WIN32_WINNT 0x0500
#include "internal.h"

#include <sys/stat.h>
#include <io.h>

#define BUFSIZE MAX_PATH

#include "texture.h"
#include FT_MODULE_H

__GLCcommonArea __glcCommonArea;
__GLCthreadArea* __glcThreadArea = NULL;

static void __glcContextUpdateHashTable(__GLCcontext *This);
static LPTSTR __glcAddCatalog(const GLCchar* inCatalog);



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 */
__GLCcontext* __glcContextCreate(GLint inContext)
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

#ifdef FT_CACHE_H
  if (FTC_Manager_New(This->library, 0, 0, 0, __glcFileOpen, NULL,
		      &This->cache)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
#endif

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;

  This->catalogList = __glcArrayCreate(sizeof(GLCchar8*));
  if (!This->catalogList) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }

  This->masterHashTable = __glcArrayCreate(sizeof(GLCchar32));
  if (!This->masterHashTable) {
    __glcArrayDestroy(This->catalogList);
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
  __glcContextUpdateHashTable(This);

  This->currentFontList.head = NULL;
  This->currentFontList.tail = NULL;
  This->fontList.head = NULL;
  This->fontList.tail = NULL;

  This->isCurrent = GL_FALSE;
  This->isInGlobalCommand = GL_FALSE;
  This->id = inContext;
  This->pendingDelete = GL_FALSE;
  This->stringState.callback = GLC_NONE;
  This->stringState.dataPointer = NULL;
  This->stringState.replacementCode = 0;
  This->stringState.stringType = GLC_UCS1;
  This->enableState.autoFont = GL_TRUE;
  This->enableState.glObjects = GL_TRUE;
  This->enableState.mipmap = GL_TRUE;
  This->enableState.hinting = GL_FALSE;
  This->enableState.extrude = GL_FALSE;
  This->enableState.kerning = GL_FALSE;
  This->renderState.resolution = 0.;
  This->renderState.renderStyle = GLC_BITMAP;
  This->bitmapMatrixStackDepth = 1;
  This->bitmapMatrix = This->bitmapMatrixStack;
  This->bitmapMatrix[0] = 1.;
  This->bitmapMatrix[1] = 0.;
  This->bitmapMatrix[2] = 0.;
  This->bitmapMatrix[3] = 1.;
  This->attribStackDepth = 0;
  This->measurementBuffer = __glcArrayCreate(12 * sizeof(GLfloat));
  if (!This->measurementBuffer) {
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
  This->isInCallbackFunc = GL_FALSE;
  This->buffer = NULL;
  This->bufferSize = 0;
  This->lastFontID = 1;
  This->vertexArray = __glcArrayCreate(2 * sizeof(GLfloat));
  if (!This->vertexArray) {
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
  This->controlPoints = __glcArrayCreate(5 * sizeof(GLfloat));
  if (!This->controlPoints) {
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
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
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }

  This->vertexIndices = __glcArrayCreate(sizeof(GLuint));
  if (!This->vertexIndices) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->endContour);
    __glcArrayDestroy(This->catalogList);
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }

  This->geomBatches = __glcArrayCreate(sizeof(__GLCgeomBatch));
  if (!This->geomBatches) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->endContour);
    __glcArrayDestroy(This->vertexIndices);
    __glcArrayDestroy(This->catalogList);
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef FT_CACHE_H
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }

  This->texture.id = 0;
  This->texture.width = 0;
  This->texture.heigth = 0;
  This->texture.bufferObjectID = 0;

  This->atlas.id = 0;
  This->atlas.width = 0;
  This->atlas.heigth = 0;
  This->atlasList.head = NULL;
  This->atlasList.tail = NULL;
  This->atlasWidth = 0;
  This->atlasHeight = 0;
  This->atlasCount = 0;

  /* The environment variable GLC_PATH is an alternate way to allow QuesoGLC
   * to access to fonts catalogs/directories.
   */
  /*Check if the GLC_PATH environment variables are exported */
  if (getenv("GLC_CATALOG_LIST") || getenv("GLC_PATH")) {
    char *path = NULL;
    char *begin = NULL;
    char *sepPos = NULL;
    char *separator = NULL;

    /* Read the paths of fonts file.
     * First, try GLC_CATALOG_LIST...
     */
    if (getenv("GLC_CATALOG_LIST"))
      path = strdup(getenv("GLC_CATALOG_LIST"));
    else if (getenv("GLC_PATH")) {
      /* Try GLC_PATH which uses the same format than PATH */
      path = strdup(getenv("GLC_PATH"));
    }

    /* Get the list separator */
    separator = getenv("GLC_LIST_SEPARATOR");
    if (!separator) {
#ifdef __WIN32__
	/* Windows can not use a colon-separated list since the colon sign is
	 * used after the drive letter. The semicolon is used for the PATH
	 * variable, so we use it for consistency.
	 */
	separator = (char *)";";
#else
	/* POSIX platforms uses colon-separated lists for the paths variables
	 * so we keep with it for consistency.
	 */
	separator = (char *)":";
#endif
    }

    if (path) {
      /* Get each path and add the corresponding masters to the current
       * context */
      LPTSTR dup = NULL;

      begin = path;
      do {
	sepPos = (char *)__glcFindIndexList(begin, 1, separator);

        if (*sepPos)
	  *(sepPos++) = 0;

        dup = __glcAddCatalog(begin);
        if (!dup) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
        }
        else {
	  if (!__glcArrayAppend(This->catalogList, &dup)) {
            __glcRaiseError(GLC_RESOURCE_ERROR);
            __glcFree(dup);
          }
        }

        begin = sepPos;
      } while (*sepPos);
      free(path);
      __glcContextUpdateHashTable(This);
    }
    else {
      /* strdup has failed to allocate memory to duplicate GLC_PATH => ERROR */
      __glcRaiseError(GLC_RESOURCE_ERROR);
    }
  }

  return This;
}



/* This function is called from FT_List_Finalize() to destroy all
 * remaining fonts
 */
static void __glcFontDestructor(FT_Memory GLC_UNUSED_ARG(inMemory),
                                void* inData, void* inUser)
{
  __GLCfont *font = (__GLCfont*)inData;
  __GLCcontext* ctx = (__GLCcontext*)inUser;

  assert(ctx);

  if (font)
    __glcFontDestroy(font, ctx);
}



/* Destructor of the object : it first destroys all the GLC objects that have
 * been created during the life of the context. Then it releases the memory
 * occupied by the GLC state struct.
 * It does not destroy GL objects associated with the GLC context since we can
 * not be sure that the current GL context is the GL context that contains the
 * GL objects designated by the GLC context that we are destroying. This could
 * happen if the user calls glcDeleteContext() after the GL context has been
 * destroyed of after the user has changed the current GL context.
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
  FT_List_Finalize(&This->currentFontList, NULL,
		   &__glcCommonArea.memoryManager, NULL);

  /* Destroy GLC_FONT_LIST */
  FT_List_Finalize(&This->fontList, __glcFontDestructor,
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

#ifdef FT_CACHE_H
  FTC_Manager_Done(This->cache);
#endif
  FT_Done_Library(This->library);
  __glcFree(This);
}



/* Return the first font in GLC_CURRENT_FONT_LIST that maps 'inCode'.
 * If there is no such font, the function returns NULL.
 * 'inCode' must be given in UCS-4 format.
 */
static __GLCfont* __glcLookupFont(GLint inCode, FT_List fontList)
{
  FT_ListNode node = NULL;

  for (node = fontList->head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)node->data;

    /* Check if the character identified by inCode exists in the font */
    if (__glcCharMapHasChar(font->charMap, inCode))
      return font;
  }
  return NULL;
}



/* Calls the callback function (does various tests to determine if it is
 * possible) and returns GL_TRUE if it has succeeded or GL_FALSE otherwise.
 * 'inCode' must be given in UCS-4 format.
 */
static GLboolean __glcCallCallbackFunc(GLint inCode,
				       __GLCcontext *inContext)
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
__GLCfont* __glcContextGetFont(__GLCcontext *This, GLint inCode)
{
  __GLCfont* font = NULL;

  /* Look for a font in the current font list */
  font = __glcLookupFont(inCode, &This->currentFontList);
  /* If a font has been found return */
  if (font)
    return font;

  /* If a callback function is defined for GLC_OP_glcUnmappedCode then call it.
   * The callback function should return GL_TRUE if it succeeds in appending to
   * GLC_CURRENT_FONT_LIST the ID of a font that maps 'inCode'.
   */
  if (__glcCallCallbackFunc(inCode, This)) {
    font = __glcLookupFont(inCode, &This->currentFontList);
    if (font)
      return font;
  }

  /* If the value of the boolean variable GLC_AUTOFONT is GL_TRUE then search
   * GLC_FONT_LIST for the first font that maps 'inCode'. If the search
   * succeeds, then append the font's ID to GLC_CURRENT_FONT_LIST.
   */
  if (This->enableState.autoFont) {
    __GLCmaster* master = NULL;

    font = __glcLookupFont(inCode, &This->fontList);
    if (font) {
      __glcAppendFont(This, font);
      return font;
    }

    master = __glcMasterMatchCode(This, inCode);
    if (!master)
      return NULL;
    font = __glcNewFontFromMaster(NULL, glcGenFontID(), master, This);
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
 * __glcCtxQueryBuffer() should be called whenever the buffer is to be used
 * in order to check if it is big enough to store infos.
 * Note that the only memory management function used below is 'realloc' which
 * means that the buffer goes bigger and bigger until it is freed. No function
 * is provided to reduce its size so it should be freed and re-allocated
 * manually in case of emergency ;-)
 */
GLCchar* __glcContextQueryBuffer(__GLCcontext *This, int inSize)
{
  GLCchar* buffer;

  buffer = This->buffer;
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



static int CALLBACK __glcEnumUpdateHashTable(ENUMLOGFONTEX* inElfe,
                                             NEWTEXTMETRICEX* inNtme,
                                             DWORD inFontType, LPARAM inData)
{
  __GLCarray* masterHashTable = (__GLCarray*)inData;
  GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(masterHashTable);
  TCHAR* ptr = NULL;
  int length = GLC_ARRAY_LENGTH(masterHashTable);
  int i = 0;
  GLCchar32 hashValue = 0;

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  hashValue = __glcHashValue(&inElfe->elfLogFont);

  /* Check if the font is already registered in the hash table */
  for (i = 0; i < length; i++) {
    if (hashTable[i] == hashValue)
      break;
  }

  /* If the font is already registered then parse the next one */
  if (i != length)
    return 1;

  /* Register the font (i.e. append its hash value to the hash table) */
  if (!__glcArrayAppend(masterHashTable, &hashValue)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  return 1;
}



/* Update the hash table that allows to convert master IDs into FontConfig
 * patterns.
 */
static void __glcContextUpdateHashTable(__GLCcontext *This)
{
  int i = 0;
  LOGFONT lfont;
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumUpdateHashTable,
                     (LPARAM)This->masterHashTable, 0);

  DeleteDC(dc);
}



static LPTSTR __glcAddCatalog(const GLCchar* inCatalog)
{
  WIN32_FIND_DATA FindFileData;
  LPTSTR DirSpec = NULL;
  size_t length = 0;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  TCHAR* ptr;

  DirSpec = (LPTSTR) __glcMalloc(BUFSIZE);
  if (!DirSpec) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Check that input is not larger than allowed */
  StringCbLength(inCatalog, BUFSIZE, &length);
  if (length > (BUFSIZE - 2)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcFree(DirSpec);
    return NULL;
  }

  StringCbCopyN(DirSpec, BUFSIZE, inCatalog, length+1);
  StringCbCatN(DirSpec, BUFSIZE, TEXT("\\*"), 2*sizeof(TCHAR));

  hFind = FindFirstFile(DirSpec, &FindFileData);
  if (hFind == INVALID_HANDLE_VALUE) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcFree(DirSpec);
    return NULL;
  }

  AddFontResourceEx(FindFileData.cFileName, FR_PRIVATE, 0);

  while (FindNextFile(hFind, &FindFileData))
    AddFontResourceEx(FindFileData.cFileName, FR_PRIVATE, 0);

  FindClose(hFind);

  ptr = (TCHAR*)DirSpec;
  ptr[length+1] = (TCHAR)'\0';

  return DirSpec;
}



/* Append a catalog to the context catalog list */
void __glcContextAppendCatalog(__GLCcontext* This, const GLCchar* inCatalog)
{
  LPTSTR DirSpec = __glcAddCatalog(inCatalog);

  if (!DirSpec)
    return;

  if (!__glcArrayAppend(This->catalogList, DirSpec)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(DirSpec);
    return;
  }
  __glcContextUpdateHashTable(This);
}



/* Prepend a catalog to the context catalog list */
void __glcContextPrependCatalog(__GLCcontext* This, const GLCchar* inCatalog)
{
  LPTSTR DirSpec = __glcAddCatalog(inCatalog);

  if (!DirSpec)
    return;

  if (!__glcArrayInsert(This->catalogList, 0, &dup)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(dup);
    return;
  }

  __glcContextUpdateHashTable(This);
}



/* Get the path of the catalog identified by inIndex */
GLCchar8* __glcContextGetCatalogPath(__GLCcontext* This, GLint inIndex)
{
  if (inIndex >= GLC_ARRAY_LENGTH(This->catalogList)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }
    
  return ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[inIndex];
}



/* Remove a catalog from the context catalog list */
void __glcContextRemoveCatalog(__GLCcontext* This, GLint inIndex)
{
  FT_ListNode node = NULL;
  int i = 0;
  WIN32_FIND_DATA FindFileData;
  LPTSTR DirSpec = NULL;
  HANDLE hFind = INVALID_HANDLE_VALUE;

  if (inIndex >= GLC_ARRAY_LENGTH(This->catalogList)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  DirSpec = ((LPTSTR*)GLC_ARRAY_DATA(This->catalogList))[inIndex];
  assert(DirSpec);
  __glcArrayRemove(This->catalogList, inIndex);
  StringCbCatN(DirSpec, BUFSIZE, TEXT("\\*"), 2*sizeof(TCHAR));

  hFind = FindFirstFile(DirSpec, &FindFileData);
  if (hFind == INVALID_HANDLE_VALUE) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcFree(DirSpec);
    return;
  }

  RemoveFontResourceEx(FindFileData.cFileName, FR_PRIVATE, 0);

  while (FindNextFile(hFind, &FindFileData))
    RemoveFontResourceEx(FindFileData.cFileName, FR_PRIVATE, 0);

  FindClose(hFind);
  __glcFree(DirSpec);

  /* Re-create the hash table from scratch */
  GLC_ARRAY_LENGTH(This->masterHashTable) = 0;
  __glcContextUpdateHashTable(This);

  /* Remove from GLC_FONT_LIST the fonts that were defined in the catalog that
   * has been removed.
   */
  for (node = This->fontList.head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)(node->data);
    __GLCmaster* master = __glcMasterCreate(font->parentMasterID, This);
    GLCchar32 hashValue = 0;
    GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(This->masterHashTable);
    int length = GLC_ARRAY_LENGTH(This->masterHashTable);
    int i = 0;

    if (!master) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      continue;
    }
    /* Check if the hash value of the master is in the hash table */
    hashValue = GLC_MASTER_HASH_VALUE(master);
    for (i = 0; i < length; i++) {
      if (hashValue == hashTable[i])
	break;
    }

    /* The font is not contained in the hash table => remove it */
    if (i == length)
      glcDeleteFont(font->id);

    __glcMasterDestroy(master);
  }
}
