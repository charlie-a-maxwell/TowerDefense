curMS = 0
id = 0
lastTime = 0
lastShot = 0

damage = 3
reload = 2000
range = 2
cost = 3
shottexture = "ice.dds"
chartexture = "tower2.dds"

function OnUpdate (deltaMS)

	shoot_tower(id, damage)
	curMS = curMS + deltaMS

end

function OnInitialize (m)
	id = m
	curMS = 0
end

function Fire (tar)

	damage = (curMS - lastTime )/300
	if damage > 1 then lastShot = curMS end
	if (damage < 1) and (curMS - lastShot > 300) then
	damage = 1
	lastShot = curMS
	end
	damage_target(tar, damage)
	lastTime = curMS

end

function SetTarget(tar)
target = tar
end

function Upgrade(d, re, ra)
damage = damage + d
range = range + ra
reload = reload + re
end
