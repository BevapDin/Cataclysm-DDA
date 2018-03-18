--[[ DEBUG ]]--

DEBUG_ENABLED = true

--[[ Named requirements ]]--

log = require("./data/mods/dda-lua-test-callback/log")

--[[ Globals initialization ]]--

LOG = log.init("./dda-lua-test-callback.log", DEBUG_ENABLED or false, DEBUG_ENABLED or false)
