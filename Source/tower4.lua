curMS = 0
id = 0
damage = 1
reload = 1000
range = 5
cost = 1
shottexture = "clear.dds"
target = -1
chartexture = "tower4.dds"

function OnUpdate (deltaMS)

curMS = curMS + deltaMS

if (curMS > reload) then
	fire_missile(id)
	curMS = curMS - reload
	end
end

function OnInitialize (m)
id = m
curMS = 0
end

function Fire(tar)
	damage_target(tar, damage)
end

function SetTarget(tar)
target = tar
end

function Upgrade(d, re, ra)
damage = damage + d
range = range + ra
reload = reload + re
end
