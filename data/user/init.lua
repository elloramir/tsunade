local keymap = require "core.keymap"
local config = require "core.config"
local style = require "core.style"

require "user.colors.fall"

config.scale = 1.5
config.draw_whitespace = true

keymap.add { ["escape"] = "core:quit" }

