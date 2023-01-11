local draw = {}

local gfx = require "system.game.Graphics"
local vector2 = require "system.utils.Vector2"
local rect = require "system.utils.Rect"
local color = require "system.utils.Color"
local orderedSelector = require "system.events.OrderedSelector"

local drawTriangle = gfx.drawTriangle
local drawTriangleGradient = gfx.drawTriangleGradient

local pi = math.pi

local themeSelector = orderedSelector.new(event.themeChanged, {
	"colorConstants",
	"colors"
})

scaleFactor = 1
uppercaseEnabled = true
darkTheme = true

draw.Font = {
	SYSTEM = {
		font = "gfx/font.png",
		fillColor = color.rgb(0, 0, 0),
		shadowColor = color.rgba(255, 255, 255, 128),
		gradient = true,
		size = 16,
	},
	VECTOR_OUTLINE = {
		font = "ttf/NotoSans-Bold.ttf",
		fillColor = color.rgb(255, 255, 255),
		outlineColor = color.rgb(0, 0, 0),
		outlineThickness = 6,
		detail = 1.5,
	},
	VECTOR = {
		font = "ttf/NotoSans-Bold.ttf",
		fillColor = color.rgb(0, 0, 0),
		outlineThickness = 0,
		detail = 1.5,
	},
	SEGOE_LIGHT = {
		font = "ttf/SegoeUI-Light.ttf",
		fillColor = color.rgb(0, 0, 0),
		outlineThickness = 0,
		detail = 1.5,
	},
	SEGOE = {
		font = "ttf/SegoeUI.ttf",
		fillColor = color.rgb(0, 0, 0),
		outlineThickness = 0,
		detail = 1.5,
	},
	SEGOE_SEMIBOLD = {
		font = "ttf/SegoeUI-SemiBold.ttf",
		fillColor = color.rgb(0, 0, 0),
		outlineThickness = 0,
		detail = 1.5,
	},
	SEGOE_BOLD = {
		font = "ttf/SegoeUI-Bold.ttf",
		fillColor = color.rgb(0, 0, 0),
		outlineThickness = 0,
		detail = 1.5,
	},
	SEGOE_MONO = {
		font = "ttf/SegoeUIMono.ttf",
		fillColor = color.rgb(0, 0, 0),
		outlineThickness = 0,
		detail = 1.5,
	},
}

draw.Color = {
	WHITE = {255, 255, 255},
	BLACK = {0, 0, 0},
	NONE = {0, 0, 0, 0},

	BG = {},
	FG = {},
	OUTLINE = {},
	FG_YELLOW = {},
	SHADOW = {},
}

local function invOff(c)
	return c
end

local function invOn(c)
	local r,g,b,a = color.getRGBA(c)
	local h,s,v,a2 = color.toHSV(color.rgba(255-r,255-g,255-b,a))
	return color.hsv(h+0.5,s,v,a2)
end

local invFunc = invOff

local hsv = color.hsv
local rgba = color.rgba

function draw.hsv(h, s, v, a)
	return invFunc(hsv(h, s, v, a or 1))
end

function draw.rgba(r, g, b, a)
	return invFunc(rgba(r, g, b, a or 255))
end

function draw.rgb(r, g, b, a)
	return invFunc(rgba(r, g, b, a or 255))
end

local function updateTheme()
	if darkTheme then
		draw.Color.FG = {255, 255, 255}
		draw.Color.BG = {30, 30, 30}
		draw.Color.OUTLINE = {128, 128, 128}
		draw.Color.FG_YELLOW = {255, 255, 200}
		draw.Color.BG_YELLOW = {30, 30, 0}
		draw.Color.SHADOW = {0, 0, 0, 128}
		invFunc = invOn
	else
		draw.Color.FG = {0, 0, 0}
		draw.Color.BG = {255, 255, 255}
		draw.Color.OUTLINE = {0, 0, 0}
		draw.Color.FG_YELLOW = {30, 30, 0}
		draw.Color.BG_YELLOW = {255, 255, 200}
		draw.Color.SHADOW = {0, 0, 0, 64}
		invFunc = invOff
	end
end

local function drawText(args, extraArgs)
	if type(args) ~= "table" then
		error("Invalid argument (expected table, got '" .. type(args) .. "')", 2)
	end

	local font = args.font

	if type(font) ~= "table" then
		error("Invalid font specified (expected table, got '" .. type(font) .. "')", 2)
	end

	local textInfo = {}

	for k, v in pairs(font) do
		if k == "fillColor" or k == "outlineColor" or k == "shadowColor" then
			v = invFunc(v)
		end
		textInfo[k] = v
	end

	for k, v in pairs(args) do
		if k ~= "font" then
			textInfo[k] = v
		end
	end

	if extraArgs then
		for k, v in pairs(extraArgs) do
			textInfo[k] = v
		end
	end

	return gfx.drawText(textInfo)
end

function draw.text(args)
	return drawText(args)
end

function draw.measureText(args)
	return drawText(args, {noDraw = true})
end

function draw.rect(r, col)
	return gfx.drawRect(r, col)
end

function draw.rectGradient(r, color1, color2, vertical)
	local topLeft, bottomRight = r:topLeft(), r:bottomRight()
	drawTriangleGradient(topLeft, r:topRight(), bottomRight, color1, vertical and color1 or color2, color2)
	drawTriangleGradient(topLeft, bottomRight, r:bottomLeft(), color1, color2, vertical and color2 or color1)
end

function draw.line(p1, p2, col, thicknessStart, thicknessEnd)
	thicknessStart = thicknessStart or 1
	thicknessEnd = thicknessEnd or thicknessStart

	local direction = p2 - p1
	local unitDirection = direction:normalize()
	local unitPerpendicular = vector2(-unitDirection.y, unitDirection.x)

	local offsetStart = unitPerpendicular * (thicknessStart * 0.5)
	local offsetEnd = unitPerpendicular * (thicknessEnd * 0.5)

	local v1 = p1 + offsetStart
	local v2 = p1 - offsetStart
	local v3 = p2 - offsetEnd
	local v4 = p2 + offsetEnd

	drawTriangle(v1, v2, v3, col)
	drawTriangle(v1, v3, v4, col)
end

function draw.lineCapped(p1, p2, col, thickness, pointiness, arrow1, arrow2)
	thickness = thickness or 1
	pointiness = pointiness or 1
	arrow1 = arrow1 or thickness
	arrow2 = arrow2 or arrow1
	draw.line(p1, p2, col, thickness)
	draw.line(p1, p1 - (p2 - p1):normalize() * pointiness * arrow1, col, arrow1, 0)
	draw.line(p2, p2 - (p1 - p2):normalize() * pointiness * arrow2, col, arrow2, 0)
end

local function circlePoint(center, radius, fraction)
	return vector2(radius, 0):rotate(fraction) + center
end

function draw.circle(center, radius, col, segments, smooth)
	smooth = smooth or false
	if (not smooth) then
		-- draw circle with the help of triangles
		segments = segments or 25
		local multiplier = 2 * pi / segments
		local v1 = center
		for i = 1, segments do
			local v2 = circlePoint(center, radius, i * multiplier)
			local v3 = circlePoint(center, radius, (i + 1) * multiplier)
			drawTriangle(v1, v2, v3, col)
		end
	else
		-- at the border, set single pixels for smoothing
		local border = 2
		local size = math.ceil(radius) + border
		local smoothing_per_one = 1 / 2
		local center_int = vector2(math.floor(center.x + 0.5), math.floor(center.y + 0.5))
		for i = -size, size do
			for j = -size, size do
				local distance = math.sqrt((center_int.x + i - center.x)^2 + (center_int.y + j - center.y)^2)
				--if (distance > radius) then
					distance = distance - radius + border
					if (distance < border) then
						local alpha = math.min(1, math.max(0, 1 - smoothing_per_one * distance))
						if (alpha > 0) then
							gfx.drawVertex(center_int + vector2(i, j), color.setA(col, color.getA(col) * alpha))
						end
					end
				--end
			end
		end
	end
end

function draw.colorWithAlpha(col, alpha)
	return {col[1], col[2], col[3], alpha}
end

function draw.setDarkThemeEnabled(enabled)
	darkTheme = enabled
	themeSelector.fire()
end

function draw.isDarkThemeEnabled()
	return darkTheme
end

updateTheme()

event.themeChanged.add("updateColorConstants", "colorConstants", updateTheme)

event.startup.add("initThemeEvent", "theme", function ()
	themeSelector.fire()
end)

return draw
