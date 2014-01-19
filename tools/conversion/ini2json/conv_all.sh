#!/bin/bash
# Convert all stats to JSON format. Run it from its directory.
#
# Missing : propulsionsounds.ini propulsiontype.ini structuremodifier.ini weaponmodifier.ini
#

BASE="../../../data"

#
# First make dictionary of used IDs
#

function dict
{
	python -t ini2dict.py ${BASE}/base/stats/$1.ini > id_dict.json
	python -t ini2json_generic.py ${BASE}/mp/stats/$1.ini  > ${BASE}/mp/stats/$1.json
}
rm -f id_dict_*.json
python ini2dict.py ${BASE}/base/stats/ecm.ini ${BASE}/base/stats/sensor.ini ${BASE}/base/stats/weapons.ini ${BASE}/base/stats/repair.ini \
	${BASE}/base/stats/construction.ini ${BASE}/base/stats/propulsion.ini ${BASE}/base/stats/structure.ini ${BASE}/base/stats/body.ini \
	${BASE}/base/stats/templates.ini ${BASE}/mp/stats/research_cam3.ini > id_dict_base.json
python ini2dict.py ${BASE}/mp/stats/ecm.ini ${BASE}/mp/stats/sensor.ini ${BASE}/mp/stats/weapons.ini ${BASE}/mp/stats/repair.ini \
	${BASE}/mp/stats/construction.ini ${BASE}/mp/stats/propulsion.ini ${BASE}/mp/stats/structure.ini ${BASE}/mp/stats/body.ini \
	${BASE}/mp/stats/templates.ini ${BASE}/mp/stats/research.ini > id_dict_mp.json

#
# Finally make the JSON files
#

function generic
{
	python -t ini2json_generic.py ${BASE}/base/stats/$1.ini id_dict_base.json  > ${BASE}/base/stats/$1.json
	python -t ini2json_generic.py ${BASE}/mp/stats/$1.ini id_dict_mp.json  > ${BASE}/mp/stats/$1.json
}
generic ecm
generic sensor
generic weapons
generic repair
generic construction
generic propulsion
generic structure
generic templates
python -t ini2json_body.py ${BASE}/base/stats/body.ini ${BASE}/base/stats/bodypropulsionimd.ini id_dict_base.json > ${BASE}/base/stats/body.json
python -t ini2json_body.py ${BASE}/mp/stats/body.ini ${BASE}/mp/stats/bodypropulsionimd.ini id_dict_base.json > ${BASE}/mp/stats/body.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam1.ini id_dict_base.json > ${BASE}/base/stats/research_cam1.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam2.ini id_dict_base.json > ${BASE}/base/stats/research_cam2.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam3.ini id_dict_base.json > ${BASE}/base/stats/research_cam3.json
python -t ini2json_research.py ${BASE}/mp/stats/research.ini id_dict_mp.json > ${BASE}/mp/stats/research.json
