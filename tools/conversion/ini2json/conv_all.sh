#!/bin/bash
# Convert all stats to JSON format. Run it from its directory.
#
# Missing : bodypropulsionimd.ini propulsionsounds.ini propulsiontype.ini structuremodifier.ini weaponmodifier.ini
#
BASE="../../../data"
function generic
{
	python ini2json_generic.py ${BASE}/base/stats/$1.ini  > ${BASE}/base/stats/$1.json
	python ini2json_generic.py ${BASE}/mp/stats/$1.ini  > ${BASE}/mp/stats/$1.json
}
generic ecm
generic sensor
generic weapons
generic repair
generic construction
generic propulsion
generic structure
generic body
generic templates
python ini2json_research.py ${BASE}/base/stats/research_cam1.ini > ${BASE}/base/stats/research_cam1.json
python ini2json_research.py ${BASE}/base/stats/research_cam2.ini > ${BASE}/base/stats/research_cam2.json
python ini2json_research.py ${BASE}/base/stats/research_cam3.ini > ${BASE}/base/stats/research_cam3.json
python ini2json_research.py ${BASE}/mp/stats/research.ini > ${BASE}/mp/stats/research.json
