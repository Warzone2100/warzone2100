/* This file is generated automatically, do not edit, change the source (stats-db2.tpl) instead. */

#include <sqlite3.h>

#line 1 "stats-db2.tpl.sql.c"
#line 7 "stats-db2.c"

/** Load the contents of the PROPULSION table from the given SQLite database.
 *
 *  @param db represents the database to load from
 *
 *  @return true if we succesfully loaded all available rows from the table,
 *          false otherwise.
 */
bool
#line 234 "stats-db2.tpl"
loadPropulsionStatsFromDB
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
		int maxSpeed;
		int propulsionType;
	} cols;

	{
		unsigned int ROW_COUNT_VAR;

		/* Prepare this SQL statement for execution */
		if (!prepareStatement(db, &stmt, "SELECT COUNT(`PROPULSION`.unique_inheritance_id) FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `PROPULSION` ON `COMPONENT`.`unique_inheritance_id` = `PROPULSION`.`unique_inheritance_id`;"))
			return false;

		/* Execute and process the results of the above SQL statement */
		if (sqlite3_step(stmt) != SQLITE_ROW
		 || sqlite3_data_count(stmt) != 1)
			goto in_statement_err;
		ROW_COUNT_VAR = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);

#line 236 "stats-db2.tpl"
		if (!statsAllocPropulsion(ROW_COUNT_VAR))
			return false;
#line 57 "stats-db2.c"
	}

	/* Prepare the query to start fetching all rows */
	if (!prepareStatement(db, &stmt,
	                          "SELECT\n"
	                              "-- Automatically generated ID to link the inheritance hierarchy.\n"
	                              "BASE.unique_inheritance_id,\n"
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

	                              "-- Max speed for the droid\n"
	                              "`PROPULSION`.`maxSpeed` AS `maxSpeed`,\n"

	                              "-- Type of propulsion used - index into PropulsionTable\n"
	                              "`PROPULSION`.`propulsionType` AS `propulsionType`\n"
	                          "FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `PROPULSION` ON `COMPONENT`.`unique_inheritance_id` = `PROPULSION`.`unique_inheritance_id`;"))
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
	cols.maxSpeed = getColNumByName(stmt, "maxSpeed");
		ASSERT(cols.maxSpeed != -1, "Column maxSpeed not found in result set!");
		if (cols.maxSpeed == -1)
			goto in_statement_err;
	cols.propulsionType = getColNumByName(stmt, "propulsionType");
		ASSERT(cols.propulsionType != -1, "Column propulsionType not found in result set!");
		if (cols.propulsionType == -1)
			goto in_statement_err;

	while (rc == SQLITE_ROW)
	{
		PROPULSION_STATS sStats, * const stats = &sStats;

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
		/* Max speed for the droid
		 */
		stats->maxSpeed = sqlite3_column_int(stmt, cols.maxSpeed);

		/* Type of propulsion used - index into PropulsionTable
		 */
		if      (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Wheeled") == 0)
			stats->propulsionType = PROPULSION_TYPE_WHEELED;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Tracked") == 0)
			stats->propulsionType = PROPULSION_TYPE_TRACKED;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Legged") == 0)
			stats->propulsionType = PROPULSION_TYPE_LEGGED;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Hover") == 0)
			stats->propulsionType = PROPULSION_TYPE_HOVER;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Ski") == 0)
			stats->propulsionType = PROPULSION_TYPE_SKI;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Lift") == 0)
			stats->propulsionType = PROPULSION_TYPE_LIFT;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Propellor") == 0)
			stats->propulsionType = PROPULSION_TYPE_PROPELLOR;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Half-Tracked") == 0)
			stats->propulsionType = PROPULSION_TYPE_HALF_TRACKED;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.propulsionType), "Jump") == 0)
			stats->propulsionType = PROPULSION_TYPE_JUMP;
		else
		{
			debug(LOG_ERROR, "Unknown enumerant (%s) for field propulsionType", (const char*)sqlite3_column_text(stmt, cols.propulsionType));
			goto in_statement_err;
		}

		{

#line 240 "stats-db2.tpl"
			stats->ref = REF_PROPULSION_START + CUR_ROW_NUM;

			// save the stats
			statsSetPropulsion(stats, CUR_ROW_NUM);

			// set the max stat values for the design screen
			if (stats->designable)
			{
				setMaxPropulsionSpeed(stats->maxSpeed);
				//setMaxComponentWeight(stats->weight);
			}
#line 252 "stats-db2.c"
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

/** Load the contents of the SENSOR table from the given SQLite database.
 *
 *  @param db represents the database to load from
 *
 *  @return true if we succesfully loaded all available rows from the table,
 *          false otherwise.
 */
bool
#line 285 "stats-db2.tpl"
loadSensorStatsFromDB
#line 278 "stats-db2.c"
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
		int range;
		int power;
		int location;
		int type;
		int time;
		int pMountGraphic;
	} cols;

	{
		unsigned int ROW_COUNT_VAR;

		/* Prepare this SQL statement for execution */
		if (!prepareStatement(db, &stmt, "SELECT COUNT(`SENSOR`.unique_inheritance_id) FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `SENSOR` ON `COMPONENT`.`unique_inheritance_id` = `SENSOR`.`unique_inheritance_id`;"))
			return false;

		/* Execute and process the results of the above SQL statement */
		if (sqlite3_step(stmt) != SQLITE_ROW
		 || sqlite3_data_count(stmt) != 1)
			goto in_statement_err;
		ROW_COUNT_VAR = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);

#line 287 "stats-db2.tpl"
		if (!statsAllocSensor(ROW_COUNT_VAR))
			return false;
#line 320 "stats-db2.c"
	}

	/* Prepare the query to start fetching all rows */
	if (!prepareStatement(db, &stmt,
	                          "SELECT\n"
	                              "-- Automatically generated ID to link the inheritance hierarchy.\n"
	                              "BASE.unique_inheritance_id,\n"
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

	                              "-- Sensor range.\n"
	                              "`SENSOR`.`range` AS `range`,\n"

	                              "-- Sensor power (put against ecm power).\n"
	                              "`SENSOR`.`power` AS `power`,\n"

	                              "-- specifies whether the Sensor is default or for the Turret.\n"
	                              "`SENSOR`.`location` AS `location`,\n"

	                              "-- used for combat\n"
	                              "`SENSOR`.`type` AS `type`,\n"

	                              "-- Time delay before the associated weapon droids 'know' where the attack is\n"
	                              "-- from.\n"
	                              "`SENSOR`.`time` AS `time`,\n"

	                              "-- The turret mount to use.\n"
	                              "`SENSOR`.`pMountGraphic` AS `pMountGraphic`\n"
	                          "FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `SENSOR` ON `COMPONENT`.`unique_inheritance_id` = `SENSOR`.`unique_inheritance_id`;"))
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
	cols.range = getColNumByName(stmt, "range");
		ASSERT(cols.range != -1, "Column range not found in result set!");
		if (cols.range == -1)
			goto in_statement_err;
	cols.power = getColNumByName(stmt, "power");
		ASSERT(cols.power != -1, "Column power not found in result set!");
		if (cols.power == -1)
			goto in_statement_err;
	cols.location = getColNumByName(stmt, "location");
		ASSERT(cols.location != -1, "Column location not found in result set!");
		if (cols.location == -1)
			goto in_statement_err;
	cols.type = getColNumByName(stmt, "type");
		ASSERT(cols.type != -1, "Column type not found in result set!");
		if (cols.type == -1)
			goto in_statement_err;
	cols.time = getColNumByName(stmt, "time");
		ASSERT(cols.time != -1, "Column time not found in result set!");
		if (cols.time == -1)
			goto in_statement_err;
	cols.pMountGraphic = getColNumByName(stmt, "pMountGraphic");
		ASSERT(cols.pMountGraphic != -1, "Column pMountGraphic not found in result set!");
		if (cols.pMountGraphic == -1)
			goto in_statement_err;

	while (rc == SQLITE_ROW)
	{
		SENSOR_STATS sStats, * const stats = &sStats;

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
		/* Sensor range.
		 */
		stats->range = sqlite3_column_int(stmt, cols.range);

		/* Sensor power (put against ecm power).
		 */
		stats->power = sqlite3_column_int(stmt, cols.power);

		/* specifies whether the Sensor is default or for the Turret.
		 */
		if      (strcmp((const char*)sqlite3_column_text(stmt, cols.location), "DEFAULT") == 0)
			stats->location = LOC_DEFAULT;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.location), "TURRET") == 0)
			stats->location = LOC_TURRET;
		else
		{
			debug(LOG_ERROR, "Unknown enumerant (%s) for field location", (const char*)sqlite3_column_text(stmt, cols.location));
			goto in_statement_err;
		}

		/* used for combat
		 */
		if      (strcmp((const char*)sqlite3_column_text(stmt, cols.type), "STANDARD") == 0)
			stats->type = STANDARD_SENSOR;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.type), "INDIRECT CB") == 0)
			stats->type = INDIRECT_CB_SENSOR;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.type), "VTOL CB") == 0)
			stats->type = VTOL_CB_SENSOR;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.type), "VTOL INTERCEPT") == 0)
			stats->type = VTOL_INTERCEPT_SENSOR;
		else if (strcmp((const char*)sqlite3_column_text(stmt, cols.type), "SUPER") == 0)
			stats->type = SUPER_SENSOR;
		else
		{
			debug(LOG_ERROR, "Unknown enumerant (%s) for field type", (const char*)sqlite3_column_text(stmt, cols.type));
			goto in_statement_err;
		}

		/* Time delay before the associated weapon droids 'know' where the attack is
		 * from.
		 */
		stats->time = sqlite3_column_int(stmt, cols.time);

		/* The turret mount to use.
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

#line 291 "stats-db2.tpl"
			stats->ref = REF_SENSOR_START + CUR_ROW_NUM;

			// save the stats
			statsSetSensor(stats, CUR_ROW_NUM);

			// set the max stat values for the design screen
			if (stats->designable)
			{
				setMaxSensorRange(stats->range);
				setMaxSensorPower(stats->power);
				setMaxComponentWeight(stats->weight);
			}
#line 569 "stats-db2.c"
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

/** Load the contents of the CONSTRUCT table from the given SQLite database.
 *
 *  @param db represents the database to load from
 *
 *  @return true if we succesfully loaded all available rows from the table,
 *          false otherwise.
 */
bool
#line 549 "stats-db2.tpl"
loadConstructStatsFromDB
#line 595 "stats-db2.c"
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

#line 551 "stats-db2.tpl"
		if (!statsAllocConstruct(ROW_COUNT_VAR))
			return false;
#line 633 "stats-db2.c"
	}

	/* Prepare the query to start fetching all rows */
	if (!prepareStatement(db, &stmt,
	                          "SELECT\n"
	                              "-- Automatically generated ID to link the inheritance hierarchy.\n"
	                              "BASE.unique_inheritance_id,\n"
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

#line 555 "stats-db2.tpl"
			stats->ref = REF_CONSTRUCT_START + CUR_ROW_NUM;

			// save the stats
			statsSetConstruct(stats, CUR_ROW_NUM);

			// set the max stat values for the design screen
			if (stats->designable)
			{
				setMaxConstPoints(stats->constructPoints);
				setMaxComponentWeight(stats->weight);
			}
#line 813 "stats-db2.c"
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
