--[[
	This file is part of Warzone 2100.
	Copyright (C) 2009  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
]]--

version(1) -- script version

defTech = {
	"R-Vehicle-Prop-Wheels",
	"R-Sys-Spade1Mk1",
	"R-Vehicle-Body01",
	"R-Comp-SynapticLink",
	"R-Cyborg-Legs01",

	"R-Wpn-MG1Mk1",
	"R-Defense-HardcreteWall",
	"R-Vehicle-Prop-Wheels",
	"R-Sys-Spade1Mk1",
	"R-Cyborg-Wpn-MG",
	"R-Defense-Pillbox01",
	"R-Defense-Tower01",
	"R-Vehicle-Body01",
	"R-Sys-Engineering01",
	"R-Struc-CommandRelay",
	"R-Vehicle-Prop-Halftracks",
	"R-Comp-CommandTurret01",
	"R-Sys-Sensor-Turret01",
	"R-Wpn-Flamer01Mk1",
	}

lives = 20
start_power = 150
wave = 0
ENEMY = 5
PLAYERS = 4

ONEPLAYERMODE = false

waves = {
	{template="BaBaPeople", amount=40, spawn=0.5, description="Scavengers", power=5},
	{template="BarbarianTrike", amount=30, spawn=0.9, description="Scavenger Trikes", power=10},
	{template="BabaBusCan", amount=15, spawn=1.5, description="Scavenger Fire Truck", power=15},
	{template="ConstructionDroid", amount=15, spawn=2, description="Trucks", power=20},
	{template="ViperMG01Wheels", amount=15, spawn=1.5, description="Viper Machinegun Wheels", power=25},
	{template="CyborgFlamer01Grd", amount=15, spawn=1, description="Flamer Cyborgs", power=30},
	{template="ViperHMGTracks", amount=15, spawn=1.5, description="Viper Heavy Machinegun Tracks", power=35},
	}

function players()
	local i = 0
	return function ()
			if i < PLAYERS then
				i = i + 1
				return i-1
			else
				return nil
			end
		end
end

function w(p) return 128*p end
responsibleFor = myResponsibility -- reads much better

function display(message)
	for player in players() do
		msg(message, ENEMY, player)
	end
end

function initialiseHost()
	local playnum, i, tech, j

	-- create alliances
	lockAllicances(false)
	for i in players() do
		for j in players() do
			createAlliance(i, j, true)
		end
	end
	lockAllicances(true)

	delayedEvent(welcome, 5)
	repeatingEvent(unfinishedBuildingsKiller, 1)
end
-- only run this script on the host
if responsibleFor(ENEMY) then
	callbackEvent(initialiseHost, CALL_GAMEINIT)
end

-- remove the chatter from the created alliances
repeatingEvent(clearConsole, 0.1)
function stopClear()
	deactivateEvent(clearConsole)
end
delayedEvent(stopClear, 2)

function welcome()
	display("Welcome to Warzone Tower Defense")
	display("You currently have "..lives.." lives")
	display("The first wave will come in 30 seconds")
	display("Good luck and good hunting!")

	delayedEvent(sendWave, 30)
	repeatingEvent(checkLifeLostEvent, 0.5)
end

function initialiseClient()
	local player

	-- show the entire map
	revealEntireMap()

	-- set our name
	setPlayerName(ENEMY, "Warzone TD")

	-- give power and starting tech
	for player in players() do
		if responsibleFor(player) then
			setPowerLevel(start_power, player)
			count = 0
			for i, tech in ipairs(defTech) do
				completeResearch(tech, player)
			end
		end
	end

	-- give money
	callbackEvent(givePower, CALL_DROID_DESTROYED)
end
callbackEvent(initialiseClient, CALL_GAMEINIT)

function spawnDroid(template, x, y)
	local droid = addDroid(template, x, y, ENEMY)
	setDroidSecondary(droid, DSO_ATTACK_LEVEL, DSS_ALEV_NEVER);
	orderDroidLoc(droid, DORDER_MOVE, w(40), w(76))
	return droid
end

function spawn()
	spawnDroid(wave_template, math.random(w(8),  w(15)), w(3))
	if not ONEPLAYERMODE then
		spawnDroid(wave_template, math.random(w(63), w(70)), w(3))
	end
	wave_amount = wave_amount - 1
	if wave_amount == 0 then
		deactivateEvent(spawn)
	end
end

function sendWave()
	wave = wave + 1
	if not waves[wave] then
		display("You win!, starting again...")
		wave = 1
	end
	display("Wave "..wave..": "..waves[wave].description.." ("..waves[wave].power.." power)")
	wave_template = waves[wave].template
	wave_amount = waves[wave].amount
	if ONEPLAYERMODE then
		-- spawn half of the droids but in the same total time
		wave_amount = wave_amount / 2
		repeatingEvent(spawn, waves[wave].spawn * 2)
	else
		repeatingEvent(spawn, waves[wave].spawn)
	end
	delayedEvent(sendWave, 60)
end

function formatLivesMessage(lives)
	if lives ~= 1 then
		return lives.." lives left"
	else
		return lives.." life left"
	end
end

function checkLifeLostEvent()
	local droid
	local to_remove = {}
	for droid in droids(ENEMY) do
		if droid.y > w(69) then
			-- aw, isn't that sad
			table.insert(to_remove, droid)
		end
	end
	for _, droid in pairs(to_remove) do
		removeDroid(droid)
		lives = lives - 1
		display(formatLivesMessage(lives))
		if lives <= 0 then
			gameOverHost()
			return
		end
	end
end

function givePower(droid, shot_by)
	local i, wave
	if droid.player ~= ENEMY then
		-- probably someone left the game
		return
	end
	if not shot_by then
		return -- was not killed but removed in another way
	end
	if responsibleFor(shot_by.player) then
		-- find out how much power we should give
		for i, wave in pairs(waves) do
			if droidHasTemplate(droid, wave.template) then
				addPower(shot_by.player, wave.power)
				return
			end
		end
		error("could not determine how much power to give")
	end
end

function unfinishedBuildingsKiller()
	local player, droid, structure, target
	for player in players() do
		target = nil
		droid = droids(player)() -- the first value from the iterator
		if droid then
			target = rawget(droid, "target") -- use rawget because it may be nil
		end
		if not target then target = {id=-1} end

		local building_to_keep = nil
		for structure in structures(player) do
			if structure.status == SS_BEING_BUILT then
				-- under construction
				if structure.id == target.id then
					-- we are building it
					if building_to_keep then
						-- destroy the previous building we found
						destroyStructure(building_to_keep)
					end
					-- keep it
					building_to_keep = structure
				else
					-- not building it
					if building_to_keep then
						-- we already have a building we like, destroy this one
						destroyStructure(structure)
					else
						-- this is the first building we find
						building_to_keep = structure
					end
				end
			end
		end
	end
end

function gameOverHost()
	deactivateEvent(checkLifeLostEvent)
	deactivateEvent(sendWave)
end

function gameOverClient()
	gameOverMessage("END", MISS_MSG, 0, false)
	deactivateEvent(gameOverCheck)
	deactivateEvent(givePower)
end

function gameOverCheck(to, from, message)
	if from == ENEMY and message == formatLivesMessage(0) then
		gameOverClient()
	end
end
callbackEvent(gameOverCheck, CALL_AI_MSG)
