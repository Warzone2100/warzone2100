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
 *  defines the so-called "Global commands" described in chapter 3.4 of the
 *  GLC specs.
 */

/** \defgroup global Global Commands
 *  Commands to create, manage and destroy GLC contexts.
 *
 *  Those commands do not use GLC context state variables and can therefore be
 *  executed successfully if the issuing thread has no current GLC context.
 *
 *  Each GLC context has a nonzero ID of type \b GLint. When a client is linked
 *  with a GLC library, the library maintains a list of IDs that contains one
 *  entry for each of the client's GLC contexts. The list is initially empty.
 *
 *  Each client thread has a private GLC context ID variable that always
 *  contains either the value zero, indicating that the thread has no current
 *  GLC context, or the ID of the thread's current GLC context. The initial
 *  value is zero.
 *
 *  When the ID of a GLC context is stored in the GLC context ID variable of a
 *  client thread, the context is said to be current to the thread. It is not
 *  possible for a GLC context to be current simultaneously to multiple
 *  threads. With the exception of the per-thread GLC error code and context ID
 *  variables, all of the GLC state variables that are used during the
 *  execution of a GLC command are stored in the issuing thread's current GLC
 *  context. To make a context current, call glcContext().
 *
 *  When a client thread issues a GLC command, the thread's current GLC context
 *  executes the command.
 *
 *  Note that the results of issuing a GL command when there is no current GL
 *  context are undefined. Because GLC issues GL commands, you must create a GL
 *  context and make it current before calling GLC.
 *
 *  All other GLC commands raise \b GLC_STATE_ERROR if the issuing thread has
 *  no current GLC context.
 */

#include "internal.h"
#include <stdlib.h>

#ifdef __GNUC__
__attribute__((constructor)) void init(void);
__attribute__((destructor)) void fini(void);
#else
void _init(void);
void _fini(void);
#endif




/* Since the common area can be accessed by any thread, this function should
 * be called before any access (read or write) to the common area. Otherwise
 * race conditions can occur. This function must also be used whenever we call
 * a function which is not reentrant (it is the case for some Fontconfig
 * entries).
 * __glcLock/__glcUnlock can be nested : they keep track of the number of
 * time they have been called and the mutex will be released as soon as
 * __glcUnlock() will be called as many time as __glcLock() was.
 */
void __glcLock(void)
{
  __GLCthreadArea *area = NULL;

  area = GLC_GET_THREAD_AREA();
  assert(area);

  if (!area->lockState)
#ifdef __WIN32__
    EnterCriticalSection(&__glcCommonArea.section);
#else
    pthread_mutex_lock(&__glcCommonArea.mutex);
#endif

  area->lockState++;
}



/* Unlock the mutex in order to allow other threads to make accesses to the
 * common area.
 * See also the note on nested calls in __glcLock's description.
 */
void __glcUnlock(void)
{
  __GLCthreadArea *area = NULL;

  area = GLC_GET_THREAD_AREA();
  assert(area);

  assert(area->lockState);
  area->lockState--;

  if (!area->lockState)
#ifdef __WIN32__
    LeaveCriticalSection(&__glcCommonArea.section);
#else
    pthread_mutex_unlock(&__glcCommonArea.mutex);
#endif
}



#if !defined(HAVE_TLS) && !defined(__WIN32__)
/* This function is called each time a pthread is cancelled or exits in order
 * to free its specific area
 */
static void __glcFreeThreadArea(void *keyValue)
{
  __GLCthreadArea *area = (__GLCthreadArea*)keyValue;
  __GLCcontext *ctx = NULL;

  if (area) {
    /* Release the context which is current to the thread, if any */
    ctx = area->currentContext;
    if (ctx)
      ctx->isCurrent = GL_FALSE;
    free(area); /* DO NOT use __glcFree() !!! */
  }
}
#endif /* !HAVE_TLS && !__WIN32__ */



/* This function is called when QuesoGLC is no longer needed.
 */
#ifdef __GNUC__
__attribute__((destructor)) void fini(void)
#else
void _fini(void)
#endif
{
  FT_ListNode node = NULL;
#if 0
  void *key = NULL;
#endif

  __glcLock();

  /* destroy remaining contexts */
  node = __glcCommonArea.contextList.head;
  while (node) {
    FT_ListNode next = node->next;
    __glcContextDestroy((__GLCcontext*)node);
    node = next;
  }

#if FC_MINOR > 2 && defined(DEBUGMODE)
  FcFini();
#endif

  __glcUnlock();
#ifdef __WIN32__
  DeleteCriticalSection(&__glcCommonArea.section);
#else
  pthread_mutex_destroy(&__glcCommonArea.mutex);
#endif

#if 0
  /* Destroy the thread local storage */
  key = pthread_getspecific(__glcCommonArea.threadKey);
  if (key)
    __glcFreeThreadArea(key);

  pthread_key_delete(__glcCommonArea.threadKey);
#endif
}



/* Routines for memory management of FreeType
 * The memory manager of our FreeType library class uses the same memory
 * allocation functions than QuesoGLC
 */
static void* __glcAllocFunc(FT_Memory GLC_UNUSED_ARG(inMemory), long inSize)
{
  return malloc(inSize);
}

static void __glcFreeFunc(FT_Memory GLC_UNUSED_ARG(inMemory), void *inBlock)
{
  free(inBlock);
}

static void* __glcReallocFunc(FT_Memory GLC_UNUSED_ARG(inMemory),
                              long GLC_UNUSED_ARG(inCurSize),
                              long inNewSize, void* inBlock)
{
  return realloc(inBlock, inNewSize);
}



/* This function is called before any function of QuesoGLC
 * is used. It reserves memory and initialiazes the library, hence the name.
 */
#ifdef __GNUC__
__attribute__((constructor)) void init(void)
#else
void _init(void)
#endif
{
#if !defined(__WIN32__) && !defined(HAVE_TLS)
  /* A temporary variable is used to store the PTHREAD_ONCE_INIT value because
   * some platforms (namely Mac OSX) define PTHREAD_ONCE_INIT as a structure
   * initialization "{.., ..}" which is not allowed to be used by C99 anywhere
   * but at variables declaration.
   */
  const pthread_once_t onceInit = PTHREAD_ONCE_INIT;
#endif

  /* Initialize fontconfig */
  if (!FcInit())
    goto FatalError;

  __glcCommonArea.versionMajor = 0;
  __glcCommonArea.versionMinor = 2;

  /* Create the thread-local storage for GLC errors */
#ifdef __WIN32__
  __glcCommonArea.threadKey = TlsAlloc();
  if (__glcCommonArea.threadKey == 0xffffffff)
    goto FatalError;
  __glcCommonArea.__glcInitThreadOnce = 0;
#elif !defined(HAVE_TLS)
  if (pthread_key_create(&__glcCommonArea.threadKey, __glcFreeThreadArea))
    goto FatalError;
  /* Now we can initialize our actual once_control variable by copying the value
   * of the temporary variable onceInit.
   */
  __glcCommonArea.__glcInitThreadOnce = onceInit;
#endif

  __glcCommonArea.memoryManager.user = NULL;
  __glcCommonArea.memoryManager.alloc = __glcAllocFunc;
  __glcCommonArea.memoryManager.free = __glcFreeFunc;
  __glcCommonArea.memoryManager.realloc = __glcReallocFunc;

  /* Initialize the list of context states */
  __glcCommonArea.contextList.head = NULL;
  __glcCommonArea.contextList.tail = NULL;

  /* Initialize the mutex for access to the contextList array */
#ifdef __WIN32__
  InitializeCriticalSection(&__glcCommonArea.section);
#else
  if (pthread_mutex_init(&__glcCommonArea.mutex, NULL))
    goto FatalError;
#endif

  return;

 FatalError:
  __glcRaiseError(GLC_RESOURCE_ERROR);

  /* Is there a better thing to do than that ? */
  perror("GLC Fatal Error");
  exit(-1);
}



#if defined __WIN32__ && !defined __GNUC__
BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
  switch(dwReason) {
  case DLL_PROCESS_ATTACH:
    _init();
    return TRUE;
  case DLL_PROCESS_DETACH:
    _fini();
    return TRUE;
  }
  return TRUE;
}
#endif



/* Get the context state corresponding to a given context ID */
static __GLCcontext* __glcGetContext(const GLint inContext)
{
  FT_ListNode node = NULL;

  __glcLock();
  for (node = __glcCommonArea.contextList.head; node; node = node->next)
    if (((__GLCcontext*)node)->id == inContext) break;

  __glcUnlock();

  return (__GLCcontext*)node;
}



/** \ingroup global
 *  This command checks whether \e inContext is the ID of one of the client's
 *  GLC context and returns \b GLC_TRUE if and only if it is the case.
 *  \param inContext The context ID to be tested
 *  \return \b GL_TRUE if \e inContext is the ID of a GLC context,
 *          \b GL_FALSE otherwise
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetAllContexts()
 *  \sa glcContext()
 */
GLboolean APIENTRY glcIsContext(GLint inContext)
{
  GLC_INIT_THREAD();

  return (__glcGetContext(inContext) ? GL_TRUE : GL_FALSE);
}



/** \ingroup global
 *  Returns the value of the issuing thread's current GLC context ID variable
 *  \return The context ID of the current thread
 *  \sa glcContext()
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 */
GLint APIENTRY glcGetCurrentContext(void)
{
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx)
    return 0;
  else
    return ctx->id;
}



/** \ingroup global
 *  Marks for deletion the GLC context identified by \e inContext. If the
 *  marked context is not current to any client thread, the command deletes
 *  the marked context immediatly. Otherwise, the marked context will be
 *  deleted during the execution of the next glcContext() command that causes
 *  it not to be current to any client thread.
 *
 *  \note glcDeleteContext() does not destroy the GL objects associated with
 *  the context \e inContext. Indeed for performance reasons, GLC does not keep
 *  track of the GL context that contains the GL objects associated with the
 *  the GLC context that is destroyed. Even if GLC would keep track of the GL
 *  context, it could lead GLC to temporarily change the GL context, delete the
 *  GL objects, then restore the correct GL context. In order not to adversely
 *  impact the performance of most of programs, it is the responsability of the
 *  user to call glcDeleteGLObjects() on a GLC context that is intended to be
 *  destroyed.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inContext is not the ID of
 *  one of the client's GLC contexts.
 *  \param inContext The ID of the context to be deleted
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 *  \sa glcContext()
 *  \sa glcGetCurrentContext()
 */
void APIENTRY glcDeleteContext(GLint inContext)
{
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  /* Lock the "Common Area" in order to prevent race conditions. Indeed, we
   * must prevent other threads to make current the context that we are
   * destroying.
   */
  __glcLock();

  /* verify if the context exists */
  ctx = __glcGetContext(inContext);

  if (!ctx) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcUnlock();
    return;
  }

  if (ctx->isCurrent)
    /* The context is current to a thread : just mark for deletion */
    ctx->pendingDelete = GL_TRUE;
  else {
    /* Remove the context from the context list then destroy it */
    FT_List_Remove(&__glcCommonArea.contextList, (FT_ListNode)ctx);
    ctx->isInGlobalCommand = GL_TRUE;
    __glcContextDestroy(ctx);
  }

  __glcUnlock();
}



/** \ingroup global
 *  Assigns the value \e inContext to the issuing thread's current GLC context
 *  ID variable. If another context is already current to the thread, no error
 *  is generated but the context is released and the context identified by
 *  \e inContext is made current to the thread.
 *
 *  Call \e glcContext with \e inContext set to zero to release a thread's
 *  current context.
 *
 *  When a GLCcontext is made current to a thread, GLC issues the commands
 *  \code
 *  glGetString(GL_VERSION);
 *  glGetString(GL_EXTENSIONS);
 *  \endcode
 *  and stores the returned strings. If there is no GL context current to the
 *  thread, the result of the above GL commands is undefined and so is the
 *  result of glcContext().
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inContext is not zero
 *  and is not the ID of one of the client's GLC contexts. \n
 *  The command raises \b GLC_STATE_ERROR if \e inContext is the ID of a GLC
 *  context that is current to a thread other than the issuing thread. \n
 *  The command raises \b GLC_STATE_ERROR if the issuing thread is executing
 *  a callback function that has been called from GLC.
 *  \param inContext The ID of the context to be made current
 *  \sa glcGetCurrentContext()
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 */
void APIENTRY glcContext(GLint inContext)
{
  __GLCcontext *currentContext = NULL;
  __GLCcontext *ctx = NULL;
  __GLCthreadArea *area = NULL;

  GLC_INIT_THREAD();

  if (inContext < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  area = GLC_GET_THREAD_AREA();
  assert(area);

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  if (inContext) {
    /* verify that the context exists */
    ctx = __glcGetContext(inContext);

    if (!ctx) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      __glcUnlock();
      return;
    }

    /* Get the current context of the issuing thread */
    currentContext = area->currentContext;

    /* Check if the issuing thread is executing a callback
     * function that has been called from GLC
     */
    if (currentContext) {
      if (currentContext->isInCallbackFunc) {
	__glcRaiseError(GLC_STATE_ERROR);
	__glcUnlock();
	return;
      }
    }

    /* Is the context already current to a thread ? */
    if (ctx->isCurrent) {
      /* If the context is current to another thread => ERROR ! */
      if (ctx != currentContext)
	__glcRaiseError(GLC_STATE_ERROR);

      /* If we get there, this means that the context 'inContext'
       * is already current to one thread (whether it is the issuing thread
       * or not) : there is nothing else to be done.
       */
      __glcUnlock();
      return;
    }

    /* Release old current context if any */
    if (currentContext)
      currentContext->isCurrent = GL_FALSE;

    /* Make the context current to the thread */
    area->currentContext = ctx;
    ctx->isCurrent = GL_TRUE;
  }
  else {
    /* inContext is null, the current thread must release its context if any */

    /* Gets the current context state */
    currentContext = area->currentContext;

    if (currentContext) {
      /* Deassociate the context from the issuing thread */
      area->currentContext = NULL;
      /* Release the context */
      currentContext->isCurrent = GL_FALSE;
    }
  }

  /* Execute pending deletion if any. Here, the variable name 'currentContext'
   * is not appropriate any more : 'currentContext' used to be the current
   * context but it has either been replaced by another one or it has been
   * released.
   */
  if (currentContext) {
    if (currentContext->pendingDelete) {
      assert(!currentContext->isCurrent);
      FT_List_Remove(&__glcCommonArea.contextList, (FT_ListNode)currentContext);
      currentContext->isInGlobalCommand = GL_TRUE;
      __glcContextDestroy(currentContext);
    }
  }

  __glcUnlock();

  /* If the issuing thread has released its context then there is no point to
   * check for OpenGL extensions.
   */
  if (!inContext)
    return;

  /* During its initialization, GLEW calls glGetString(GL_VERSION) and
   * glGetString(GL_EXTENSIONS) so, not only glewInit() allows to determine the
   * available extensions, but it also allows to be conformant with GLC specs
   * which require to issue those GL commands. 
   *
   * Notice however that not all OpenGL implementations return a result if the
   * function glGetString() is called while no GL context is bound to the
   * current thread. Since this behaviour is not required by the GL specs, such
   * calls may just fail or lead to weird results or even crash the app (on
   * Mac OSX). It is the user's responsibility to make sure that it does not
   * happen since the GLC specs state clearly that : 
   *   1. glGetString() is called by glcContext()
   *   2. the behaviour of GLC is undefined if no GL context is current while
   *      issuing GL commands.
   */
  if (glewInit() != GLEW_OK)
    __glcRaiseError(GLC_RESOURCE_ERROR);
}



/** \ingroup global
 *  Generates a new GLC context and returns its ID.
 *  \return The ID of the new context
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 *  \sa glcContext()
 *  \sa glcGetCurrentContext()
 */
GLint APIENTRY glcGenContext(void)
{
  int newContext = 0;
  __GLCcontext *ctx = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Create a new context */
  ctx = __glcContextCreate(0);
  if (!ctx)
    return 0;

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  /* Search for the first context ID that is unused */
  if (__glcCommonArea.contextList.tail)
    newContext = ((__GLCcontext*)__glcCommonArea.contextList.tail)->id;

  ctx->id = newContext + 1;

  node = (FT_ListNode)ctx;
  node->data = ctx;
  FT_List_Add(&__glcCommonArea.contextList, node);

  __glcUnlock();

  return ctx->id;
}



/** \ingroup global
 *  Returns a zero terminated array of GLC context IDs that contains one entry
 *  for each of the client's GLC contexts. GLC uses the ISO C library command
 *  \c malloc to allocate the array. The client should use the ISO C library
 *  command \c free to deallocate the array when it is no longer needed.
 *  \return The pointer to the array of context IDs.
 *  \sa glcContext()
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetCurrentContext()
 *  \sa glcIsContext()
 */
GLint* APIENTRY glcGetAllContexts(void)
{
  int count = 0;
  GLint* contextArray = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Count the number of existing contexts (whether they are current to a
   * thread or not).
   */
  __glcLock();
  for (node = __glcCommonArea.contextList.head, count = 0; node;
       node = node->next, count++);

  /* Allocate memory to store the array (including the zero termination value)*/
  contextArray = (GLint *)__glcMalloc(sizeof(GLint) * (count+1));
  if (!contextArray) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcUnlock();
    return NULL;
  }

  /* Array must be null-terminated */
  contextArray[count] = 0;

  /* Copy the context IDs to the array */
  for (node = __glcCommonArea.contextList.tail; node; node = node->prev)
    contextArray[--count] = ((__GLCcontext*)node)->id;

  __glcUnlock();

  return contextArray;
}



/** \ingroup global
 *  Retrieves the value of the issuing thread's GLC error code variable,
 *  assigns the value \b GLC_NONE to that variable, and returns the retrieved
 *  value.
 *  \note In contrast to the GL function \c glGetError, \e glcGetError only
 *  returns one error, not a list of errors.
 *  \return An error code from the table below : \n\n
 *   <center>
 *   <table>
 *     <caption>Error codes</caption>
 *     <tr>
 *       <td>Name</td> <td>Enumerant</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_NONE</b></td> <td>0x0000</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_PARAMETER_ERROR</b></td> <td>0x0040</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_RESOURCE_ERROR</b></td> <td>0x0041</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_STATE_ERROR</b></td> <td>0x0042</td>
 *     </tr>
 *   </table>
 *   </center>
 */
GLCenum APIENTRY glcGetError(void)
{
  GLCenum error = GLC_NONE;
  __GLCthreadArea * area = NULL;

  GLC_INIT_THREAD();

  area = GLC_GET_THREAD_AREA();
  assert(area);

  error = area->errorState;
  __glcRaiseError(GLC_NONE);
  return error;
}
