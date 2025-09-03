local keymap = require "core.keymap"
local config = require "core.config"
local style = require "core.style"

require "user.colors.fall"

config.draw_whitespace = true

keymap.add { ["ctrl+escape"] = "core:quit" }

