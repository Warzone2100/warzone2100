/** \file ignorecase.h */

/**
 * \mainpage PhysicsFS ignorecase
 *
 * This is an extension to PhysicsFS to let you handle files in a
 *  case-insensitive manner, regardless of what sort of filesystem or
 *  archive they reside in. It does this by enumerating directories as
 *  needed and manually locating matching entries.
 *
 * Please note that this brings with it some caveats:
 *  - On filesystems that are case-insensitive to start with, such as those
 *    used on Windows or MacOS, you are adding extra overhead.
 *  - On filesystems that are case-sensitive, you might select the wrong dir
 *    or file (which brings security considerations and potential bugs). This
 *    code favours exact case matches, but you will lose access to otherwise
 *    duplicate filenames, or you might go down a wrong directory tree, etc.
 *    In practive, this is rarely a problem, but you need to be aware of it.
 *  - This doesn't do _anything_ with the write directory; you're on your
 *    own for opening the right files for writing. You can sort of get around
 *    this by adding your write directory to the search path, but then the
 *    interpolated directory tree can screw you up even more.
 *
 * This code should be considered an aid for legacy code. New development
 *  shouldn't do dumbass things that require this aid in the first place.  :)
 *
 * Usage: Set up PhysicsFS as you normally would, then use
 *  PHYSFSEXT_locateCorrectCase() to get a "correct" pathname to pass to
 *  functions like PHYSFS_openRead(), etc.
 *
 * License: this code is public domain. I make no warranty that it is useful,
 *  correct, harmless, or environmentally safe.
 *
 * This particular file may be used however you like, including copying it
 *  verbatim into a closed-source project, exploiting it commercially, and
 *  removing any trace of my name from the source (although I hope you won't
 *  do that). I welcome enhancements and corrections to this file, but I do
 *  not require you to send me patches if you make changes. This code has
 *  NO WARRANTY.
 *
 * Unless otherwise stated, the rest of PhysicsFS falls under the zlib license.
 *  Please see LICENSE in the root of the source tree.
 *
 *  \author Ryan C. Gordon.
 */


/**
 * \fn int PHYSFSEXT_locateCorrectCase(char *buf)
 * \brief Find an existing filename with matching case.
 *
 * This function will look for a path/filename that matches the passed in
 *  buffer. Each element of the buffer's path is checked for a
 *  case-insensitive match. The buffer must specify a null-terminated string
 *  in platform-independent notation.
 *
 * Please note results may be skewed differently depending on whether symlinks
 *  are enabled or not.
 *
 * Each element of the buffer is overwritten with the actual case of an
 *  existing match. If there is no match, the search aborts and reports an
 *  error. Exact matches are favored over case-insensitive matches.
 *
 * THIS IS RISKY. Please do not use this function for anything but crappy
 *  legacy code.
 *
 *   \param buf Buffer with null-terminated string of path/file to locate.
 *               This buffer will be modified by this function.
 *  \return zero if match was found, -1 if the final element (the file itself)
 *               is missing, -2 if one of the parent directories is missing.
 */
int PHYSFSEXT_locateCorrectCase(char *buf);

/* end of ignorecase.h ... */

