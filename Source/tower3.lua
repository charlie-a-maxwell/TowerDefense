curMS = 0
id = 0
damage = 1
reload = 1000
range = 2
cost = 2
shottexture = "ice.dds"
chartexture = "tower3.dds"

function OnUpdate (deltaMS)

curMS = curMS + deltaMS

if (curMS > reload) then
	shoot_tower(id, damage)
	curMS = curMS - reload
	end
end

function OnInitialize (m)
id = m
curMS = 0
end

function Fire(tar)
	slow_target(tar)
end

function SetTarget(tar)
target = tar
end

function Upgrade(d, re, ra)
damage = damage + d
range = range + ra
reload = reload + re
end
