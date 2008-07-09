#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "$header"
#include "lib/sqlite3/sqlite3.h"
#include <stdarg.h>

static bool prepareStatement(sqlite3* db, sqlite3_stmt** stmt, const char* fmt, ...)
{
        bool retval = true;
        va_list ap;
        char* statement;

        va_start(ap, fmt);
                statement = sqlite3_vmprintf(fmt, ap);
        va_end(ap);

        if (statement == NULL)
        {
                debug(LOG_ERROR, "Out of memory!");
                return false;
        }

        if (sqlite3_prepare_v2(db, statement, -1, stmt, NULL) != SQLITE_OK)
        {
                debug(LOG_ERROR, "SQL error (while preparing statement \"%s\"): %s", statement, sqlite3_errmsg(db));
                retval = false;
        }

        // Free the memory from our constructed SQL statement
        sqlite3_free(statement);
        return retval;
}

/** Retrieves the column number of the column with the given name.
 *
 *  @param stmt SQL statement result to search the column names of
 *  @param name the name of the column to search for
 *
 *  @return a column index number. The left-most column starts at zero (0), and
 *          then gets incremented by one (1) for every column to the right.
 *          -1 is returned if the given column couldn't be found.
 */
static int getColNumByName(sqlite3_stmt* stmt, const char* name)
{
	const int columns = sqlite3_column_count(stmt);
	int colnum;

	for (colnum = 0; colnum < columns; ++colnum)
	{
		// If we found a column with the given name, return its number
		if (strcmp(name, sqlite3_column_name(stmt, colnum)) == 0)
			return colnum;
	}

	// We didn't find any column for the given name, so return -1
	return -1;
}
