local colorScale = {}

local draw = require "luavis.vis.Draw"
local vector2 = require "system.utils.Vector2"

local min = math.min
local max = math.max
local floor = math.floor

local function lerpColors(a, b, value)
	return {
		a[1] * (1 - value) + b[1] * value,
		a[2] * (1 - value) + b[2] * value,
		a[3] * (1 - value) + b[3] * value,
		a[4] * (1 - value) + b[4] * value
	}
end

function colorScale.new(colors, rangeMin, rangeMax)
	rangeMin = rangeMin or 0
	rangeMax = rangeMax or 1
	local colorList = {}
	local colorCount = #colors
	for i = 1, #colors do
		local color = colors[i]
		colorList[i] = {color[1], color[2], color[3], color[4] or 255}
	end

	-- Add one-past-end entry to handle input values of 1 without extra branch
	colorList[colorCount + 1] = colorList[colorCount]

	local function sample(value)
		value = ((value - rangeMin) / (rangeMax - rangeMin)) * (colorCount - 1)
		local index = min(max(floor(value), 0), colorCount - 1)
		return lerpColors(colorList[index + 1], colorList[index + 2], value % 1)
	end

	local function render(boundingBox, outlineThickness, labels)
		outlineThickness = outlineThickness or min(boundingBox.w, boundingBox.h) * 0.1

		draw.rect(boundingBox:grow(outlineThickness), draw.Color.OUTLINE)

		local vertical = boundingBox.w < boundingBox.h

		if vertical then
			local r = boundingBox:scale(vector2(1, 1 / (colorCount - 1)))
			for i = 1, #colorList - 2 do
				draw.rectGradient(r, colorList[i], colorList[i + 1], true)
				r.y = r.y + r.h
			end
		else
			local r = boundingBox:scale(vector2(1 / (colorCount - 1), 1))
			for i = 1, #colorList - 2 do
				draw.rectGradient(r, colorList[i], colorList[i + 1], false)
				r.x = r.x + r.w
			end
		end

		if labels then
			local labelBox = boundingBox:grow(outlineThickness)
			local margin = vertical and vector2(outlineThickness, 0) or vector2(0, -outlineThickness)
			local posMin = (vertical and labelBox:topRight() or labelBox:topLeft()) + margin
			local posMax = (vertical and labelBox:bottomRight() or labelBox:topRight()) + margin
			local alignMin = vertical and vector2(0, 0) or vector2(0, 1)
			local alignMax = vertical and vector2(0, 1) or vector2(1, 1)
			local invLabelCount = 1 / max(1, #labels - 1)
			for i = 1, #labels do
				local pos = vector2.lerp(posMin, posMax, (i - 1) * invLabelCount)
				local align = vector2.lerp(alignMin, alignMax, (i - 1) * invLabelCount)
				draw.text {
					text = labels[i],
					font = draw.Font.SYSTEM,
					x = pos.x,
					y = pos.y,
					alignX = align.x,
					alignY = align.y,
				}
			end
		end
	end

	return setmetatable({}, {
		__index = {
			sample = sample,
			render = render,
		},
		__call = function (tbl, value)
			return sample(value)
		end
	})
end

return colorScale
