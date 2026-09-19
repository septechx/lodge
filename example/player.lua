local MOVE_SPEED = 4.0
local SPRINT_MULT = 3.0
local MOUSE_SENS = 0.0025
local PITCH_LIMIT = math.rad(89.0)

local yaw = 0.0
local pitch = 0.0
local initialized = false

local function clamp(v, lo, hi)
	if v < lo then
		return lo
	end
	if v > hi then
		return hi
	end
	return v
end

local function keyDown(...)
	for i = 1, select("#", ...) do
		if input.isKeyDown(select(i, ...)) then
			return true
		end
	end
	return false
end

local function quatNormalize(q)
	local len = math.sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z)
	return { w = q.w / len, x = q.x / len, y = q.y / len, z = q.z / len }
end

local function quatMul(a, b)
	return {
		w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
		x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
	}
end

local function quatFromAxisAngle(ax, ay, az, rad)
	local len = math.sqrt(ax * ax + ay * ay + az * az)
	if len < 1e-6 then
		return { w = 1.0, x = 0.0, y = 0.0, z = 0.0 }
	end
	local half = rad * 0.5
	local s = math.sin(half) / len
	return { w = math.cos(half), x = ax * s, y = ay * s, z = az * s }
end

local function quatRotate(q, v)
	local qv = quatMul(q, { w = 0.0, x = v.x, y = v.y, z = v.z })
	local c = { w = q.w, x = -q.x, y = -q.y, z = -q.z }
	local r = quatMul(qv, c)
	return { x = r.x, y = r.y, z = r.z }
end

local function initFromCurrentRotation()
	local rot = self:getRotation()
	if rot == nil then
		return
	end
	local q = quatNormalize(rot)
	local f = quatRotate(q, { x = 0.0, y = 0.0, z = -1.0 })
	yaw = math.atan2(-f.x, -f.z)
	pitch = math.asin(clamp(f.y, -1.0, 1.0))
end

function Start()
	input.setCursorMode("disabled")
end

function Update(dt)
	if not initialized then
		initFromCurrentRotation()
		initialized = true
	end

	local md = input.mouseDelta()
	yaw = yaw - md.x * MOUSE_SENS
	pitch = clamp(pitch - md.y * MOUSE_SENS, -PITCH_LIMIT, PITCH_LIMIT)

	local qYaw = quatFromAxisAngle(0.0, 1.0, 0.0, yaw)
	local qPitch = quatFromAxisAngle(1.0, 0.0, 0.0, pitch)
	self:setRotation(quatNormalize(quatMul(qYaw, qPitch)))

	local cp = math.cos(pitch)
	local sp = math.sin(pitch)
	local cy = math.cos(yaw)
	local sy = math.sin(yaw)
	local forward = { x = -sy * cp, y = sp, z = -cy * cp }
	local right = { x = cy, y = 0.0, z = -sy }

	local fb = 0.0
	if keyDown("W") then
		fb = fb + 1.0
	end
	if keyDown("S") then
		fb = fb - 1.0
	end
	local strafe = 0.0
	if keyDown("D") then
		strafe = strafe + 1.0
	end
	if keyDown("A") then
		strafe = strafe - 1.0
	end
	local vertical = 0.0
	if keyDown("space") then
		vertical = vertical + 1.0
	end
	if keyDown("control", "rightcontrol") then
		vertical = vertical - 1.0
	end

	local mx = forward.x * fb + right.x * strafe
	local my = forward.y * fb + vertical
	local mz = forward.z * fb + right.z * strafe

	local len = math.sqrt(mx * mx + my * my + mz * mz)
	if len > 1.0 then
		mx = mx / len
		my = my / len
		mz = mz / len
	end

	local speed = MOVE_SPEED
	if keyDown("shift", "rightshift") then
		speed = speed * SPRINT_MULT
	end

	local step = speed * dt
	self:translate({
		x = mx * step,
		y = my * step,
		z = mz * step,
	})
end
