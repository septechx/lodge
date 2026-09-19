function Start()
	log.info("test.lua loaded")
end

function Update(dt)
	self:translate({ x = 0.0, y = 0.0, z = dt * 1.0 })
	self:rotateAxisAngle({ x = 0.0, y = 0.0, z = 1.0 }, math.rad(dt * 45.0))
end
