function Update(dt)
	local car = scene.findByName("Car")
	if car ~= nil then
		local x, y, z = scene.getPosition(car)
		scene.setPosition(car, x, y, z + dt * 1.0)
	end
end
