function Start()
	log.info("test.lua loaded")
end

function Update(dt)
	local car = scene.findByName("Car")
	if car then
		car:translate({ x = 0.0, y = 0.0, z = dt * 1.0 })
		car:rotateAxisAngle({ x = 0.0, y = 0.0, z = 1.0 }, math.rad(dt * 45.0))
	end
end
