#!/bin/bash
# Convert all stats to JSON format. Run it from its directory.

BASE="../../../data"

function generic
{
	python -t ini2json_generic.py ${BASE}/base/stats/$1.ini > ${BASE}/base/stats/$1.json
	python -t ini2json_generic.py ${BASE}/mp/stats/$1.ini > ${BASE}/mp/stats/$1.json
}
generic ecm
generic sensor
generic weapons
generic repair
generic construction
generic propulsion
generic structure
generic templates
python -t ini2json_generic.py ${BASE}/base/stats/features.ini > ${BASE}/base/stats/features.json
python -t ini2json_generic.py ${BASE}/base/stats/brain.ini > ${BASE}/base/stats/brain.json
python -t ini2json_body.py ${BASE}/base/stats/body.ini ${BASE}/base/stats/bodypropulsionimd.ini > ${BASE}/base/stats/body.json
python -t ini2json_body.py ${BASE}/mp/stats/body.ini ${BASE}/mp/stats/bodypropulsionimd.ini > ${BASE}/mp/stats/body.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam1.ini > ${BASE}/base/stats/research_cam1.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam1.ini > research_cam1.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam2.ini > ${BASE}/base/stats/research_cam2.json
python -t ini2json_research.py ${BASE}/base/stats/research_cam3.ini > ${BASE}/base/stats/research_cam3.json
python -t ini2json_research.py ${BASE}/mp/stats/research.ini > ${BASE}/mp/stats/research.json

#
# Now convert maps (only multiplayer, campaign keeps old crap binary format for now)
#

function map
{
	python -t ini2json_save.py ${1}/droid.ini > ${1}/droid.json
	python -t ini2json_save.py ${1}/feature.ini > ${1}/feature.json
	python -t ini2json_save.py ${1}/struct.ini > ${1}/struct.json
}
for i in $(find ${BASE} -name "[1234567890]c-*" -type d); do
    map "$i"
done

#
# Convert challenges
#

python ini2json_challenge.py ${BASE}/mp/challenges/hidebehind.ini > ${BASE}/mp/challenges/hidebehind.json
python ini2json_challenge.py ${BASE}/mp/challenges/noplace.ini > ${BASE}/mp/challenges/noplace.json
python ini2json_challenge.py ${BASE}/mp/challenges/b2basics.ini > ${BASE}/mp/challenges/b2basics.json

# Convert AIs

for i in $(find ${BASE} -name "*.ai"); do
	python ini2json_challenge.py $i > ${i%.*}.json
done

# Finally, some JSON files we should fix later... horrible hacks!!

function hack
{
	python -t ini2json_challenge.py ${BASE}/base/stats/$1.ini > ${BASE}/base/stats/$1.json
	python -t ini2json_challenge.py ${BASE}/mp/stats/$1.ini > ${BASE}/mp/stats/$1.json
}
hack propulsiontype
hack propulsionsounds
hack weaponmodifier
hack structuremodifier
python -t ini2json_generic.py ${BASE}/base/stats/terraintable.ini > ${BASE}/base/stats/terraintable.json
python -t ini2json_generic.py ${BASE}/base/messages/messages.rmsg > ${BASE}/base/messages/messages.json

function rmsg
{
	cat $1.rmsg | sed 's/\\//g'| sed 's/_(/ /g' | sed 's/)//g' > _tmp
	python -t ini2json_generic.py _tmp > $1.json
	rm _tmp
}

rmsg ${BASE}/base/messages/resmessages1
rmsg ${BASE}/base/messages/resmessages12
rmsg ${BASE}/base/messages/resmessages3
rmsg ${BASE}/base/messages/resmessagesall
rmsg ${BASE}/base/messages/resmessages23
rmsg ${BASE}/base/messages/resmessages2

rmsg ${BASE}/mp/messages/resmessages1
rmsg ${BASE}/mp/messages/resmessages12
rmsg ${BASE}/mp/messages/resmessages3
rmsg ${BASE}/mp/messages/resmessagesall
rmsg ${BASE}/mp/messages/resmessages23
rmsg ${BASE}/mp/messages/resmessages2
