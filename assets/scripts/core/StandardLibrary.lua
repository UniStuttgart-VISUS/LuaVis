local standardLibrary = {}

for _, name in ipairs {"package", "coroutine", "string", "os", "math", "table", "debug", "bit", "io", "ffi", "jit"} do
	standardLibrary[name] = require(name)
end

for _, name in ipairs {"assert", "collectgarbage", "dofile", "error", "gcinfo", "getfenv", "getmetatable", "ipairs",
	"load", "loadfile", "loadstring", "module", "newproxy", "next", "pairs", "pcall", "print", "rawequal", "rawget",
	"rawset", "require", "select", "setfenv", "setmetatable", "tonumber", "tostring", "type", "unpack", "xpcall"}
do
	standardLibrary[name] = _G[name]
end

return standardLibrary
