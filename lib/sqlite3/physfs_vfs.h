/*
	Copyright (c) 2008  Giel van Schijndel

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	   distribution.
*/

#ifndef __INCLUDED_PHYSICSFS_SQLITE3_VFS_H__
#define __INCLUDED_PHYSICSFS_SQLITE3_VFS_H__

/** Registers the PhysicsFS implementation of SQLite's VFS with name "physfs"
 *  \param makeDefault specifies whether this VFS should be made
 *                     the default VFS, non-zero to make it the default,
 *                     zero to leave whatever is currently the default as the
 *                     default.
 *  \note If you want to make this VFS implementation the default if you didn't
 *        made it the default the first time, or registered another VFS to be
 *        the default as well, then you can simply call this function again with
 *        \c makeDefault non-zero.
 */
extern void sqlite3_register_physfs_vfs(int makeDefault);

/** Unregisters the PhysicsFS implementation of SQLite's VFS from
 *  SQLite's VFS-list.
 *  \note This will _not_ affect currently opened database connections,
 *        only new ones.
 */
extern void sqlite3_unregister_physfs_vfs(void);

#endif // __INCLUDED_PHYSICSFS_SQLITE3_VFS_H__
