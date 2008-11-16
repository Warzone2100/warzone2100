/* This file is generated automatically, do not edit, change the source (stats-db2.tpl) instead. */

#include <sqlite3.h>

#line 1 "stats-db2.tpl.sql.c"
#line 7 "stats-db2.c"

/** Load the contents of the CONSTRUCT table from the given SQLite database.
 *
 *  @param db represents the database to load from
 *
 *  @return true if we succesfully loaded all available rows from the table,
 *          false otherwise.
 */
bool
#line 418 "stats-db2.tpl"
loadConstructStatsFromDB
#line 19 "stats-db2.c"
	(sqlite3* db)
{
	bool retval = false;
	sqlite3_stmt* stmt;
	int rc;
	unsigned int CUR_ROW_NUM = 0;
	struct
	{
		int unique_inheritance_id;
		int pName;
		int buildPower;
		int buildPoints;
		int weight;
		int body;
		int designable;
		int pIMD;
		int constructPoints;
		int pMountGraphic;
	} cols;

	{
		unsigned int ROW_COUNT_VAR;

		/* Prepare this SQL statement for execution */
		if (!prepareStatement(db, &stmt, "SELECT COUNT(`CONSTRUCT`.unique_inheritance_id) FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `CONSTRUCT` ON `COMPONENT`.`unique_inheritance_id` = `CONSTRUCT`.`unique_inheritance_id`;"))
			return false;

		/* Execute and process the results of the above SQL statement */
		if (sqlite3_step(stmt) != SQLITE_ROW
		 || sqlite3_data_count(stmt) != 1)
			goto in_statement_err;
		ROW_COUNT_VAR = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);

#line 420 "stats-db2.tpl"
		if (!statsAllocConstruct(ROW_COUNT_VAR))
			return false;
#line 57 "stats-db2.c"
	}

	/* Prepare the query to start fetching all rows */
	if (!prepareStatement(db, &stmt,
	                          "SELECT\n"
	                              "-- Automatically generated ID to link the inheritance hierarchy.\n"
	                              "BASE.unique_inheritance_id,\n"
	                              "-- Unique ID of the item\n"
	                              "-- Unique language independant name that can be used to identify a specific\n"
	                              "-- stats instance\n"
	                              "`BASE`.`pName` AS `pName`,\n"

	                              "-- Power required to build this component\n"
	                              "`COMPONENT`.`buildPower` AS `buildPower`,\n"

	                              "-- Build points (which are rate-limited in the construction units) required\n"
	                              "-- to build this component.\n"
	                              "`COMPONENT`.`buildPoints` AS `buildPoints`,\n"

	                              "-- Weight of this component\n"
	                              "`COMPONENT`.`weight` AS `weight`,\n"

	                              "-- Body points of this component\n"
	                              "`COMPONENT`.`body` AS `body`,\n"

	                              "-- Indicates whether this component is \"designable\" and can thus be used in\n"
	                              "-- the design screen.\n"
	                              "`COMPONENT`.`designable` AS `designable`,\n"

	                              "-- The \"base\" IMD model representing this component in 3D space.\n"
	                              "`COMPONENT`.`pIMD` AS `pIMD`,\n"

	                              "-- The number of points contributed each cycle\n"
	                              "`CONSTRUCT`.`constructPoints` AS `constructPoints`,\n"

	                              "-- The turret mount to use\n"
	                              "`CONSTRUCT`.`pMountGraphic` AS `pMountGraphic`\n"
	                          "FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `CONSTRUCT` ON `COMPONENT`.`unique_inheritance_id` = `CONSTRUCT`.`unique_inheritance_id`;"))
		return false;

	/* Fetch the first row */
	if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
	{
		/* Apparently we fetched no rows at all, this is a non-failure, terminal condition. */
		sqlite3_finalize(stmt);
		return true;
	}

	/* Fetch and cache column numbers */
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	cols.unique_inheritance_id = getColNumByName(stmt, "unique_inheritance_id");
		ASSERT(cols.unique_inheritance_id != -1, "Column unique_inheritance_id not found in result set!");
		if (cols.unique_inheritance_id == -1)
			goto in_statement_err;
	cols.pName = getColNumByName(stmt, "pName");
		ASSERT(cols.pName != -1, "Column pName not found in result set!");
		if (cols.pName == -1)
			goto in_statement_err;
	/* END of inherited "BASE" definition */
	cols.buildPower = getColNumByName(stmt, "buildPower");
		ASSERT(cols.buildPower != -1, "Column buildPower not found in result set!");
		if (cols.buildPower == -1)
			goto in_statement_err;
	cols.buildPoints = getColNumByName(stmt, "buildPoints");
		ASSERT(cols.buildPoints != -1, "Column buildPoints not found in result set!");
		if (cols.buildPoints == -1)
			goto in_statement_err;
	cols.weight = getColNumByName(stmt, "weight");
		ASSERT(cols.weight != -1, "Column weight not found in result set!");
		if (cols.weight == -1)
			goto in_statement_err;
	cols.body = getColNumByName(stmt, "body");
		ASSERT(cols.body != -1, "Column body not found in result set!");
		if (cols.body == -1)
			goto in_statement_err;
	cols.designable = getColNumByName(stmt, "designable");
		ASSERT(cols.designable != -1, "Column designable not found in result set!");
		if (cols.designable == -1)
			goto in_statement_err;
	cols.pIMD = getColNumByName(stmt, "pIMD");
		ASSERT(cols.pIMD != -1, "Column pIMD not found in result set!");
		if (cols.pIMD == -1)
			goto in_statement_err;
	/* END of inherited "COMPONENT" definition */
	cols.constructPoints = getColNumByName(stmt, "constructPoints");
		ASSERT(cols.constructPoints != -1, "Column constructPoints not found in result set!");
		if (cols.constructPoints == -1)
			goto in_statement_err;
	cols.pMountGraphic = getColNumByName(stmt, "pMountGraphic");
		ASSERT(cols.pMountGraphic != -1, "Column pMountGraphic not found in result set!");
		if (cols.pMountGraphic == -1)
			goto in_statement_err;

	while (rc == SQLITE_ROW)
	{
		CONSTRUCT_STATS sStats, * const stats = &sStats;

		memset(stats, 0, sizeof(*stats));

		/* BEGIN of inherited "COMPONENT" definition */
		/* BEGIN of inherited "BASE" definition */
		/* Unique ID of the item
		 */

		/* Unique language independant name that can be used to identify a specific
		 * stats instance
		 */
		stats->pName = strdup((const char*)sqlite3_column_text(stmt, cols.pName));
		if (stats->pName == NULL)
		{
			debug(LOG_ERROR, "Out of memory");
			abort();
			goto in_statement_err;
		}

		/* END of inherited "BASE" definition */
		/* Power required to build this component
		 */
		stats->buildPower = sqlite3_column_int(stmt, cols.buildPower);

		/* Build points (which are rate-limited in the construction units) required
		 * to build this component.
		 */
		stats->buildPoints = sqlite3_column_int(stmt, cols.buildPoints);

		/* Weight of this component
		 */
		stats->weight = sqlite3_column_int(stmt, cols.weight);

		/* Body points of this component
		 */
		stats->body = sqlite3_column_int(stmt, cols.body);

		/* Indicates whether this component is "designable" and can thus be used in
		 * the design screen.
		 */
		stats->designable = sqlite3_column_int(stmt, cols.designable) ? true : false;

		/* The "base" IMD model representing this component in 3D space.
		 */
		if (sqlite3_column_type(stmt, cols.pIMD) != SQLITE_NULL)
		{
			stats->pIMD = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, cols.pIMD));
		}
		else
		{
			stats->pIMD = NULL;
		}

		/* END of inherited "COMPONENT" definition */
		/* The number of points contributed each cycle
		 */
		stats->constructPoints = sqlite3_column_int(stmt, cols.constructPoints);

		/* The turret mount to use
		 */
		if (sqlite3_column_type(stmt, cols.pMountGraphic) != SQLITE_NULL)
		{
			stats->pMountGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, cols.pMountGraphic));
		}
		else
		{
			stats->pMountGraphic = NULL;
		}

		{

#line 424 "stats-db2.tpl"
			stats->ref = REF_CONSTRUCT_START + CUR_ROW_NUM;
			
			// save the stats
			statsSetConstruct(stats, CUR_ROW_NUM);
			
			// set the max stat values for the design screen
			if (stats->designable)
			{
				setMaxConstPoints(stats->constructPoints);
				setMaxComponentWeight(stats->weight);
			}
#line 238 "stats-db2.c"
		}

		/* Retrieve the next row */
		rc = sqlite3_step(stmt);
		++CUR_ROW_NUM;
	}

	retval = true;

in_statement_err:
	sqlite3_finalize(stmt);

	return retval;
}
