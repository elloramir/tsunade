local keymap = require "core.keymap"
local config = require "core.config"
local style = require "core.style"

require "user.colors.fall"

config.draw_whitespace = true
config.tab_type = "soft" -- set to hard to use tabs instead
config.indent_size = 4

keymap.add { ["ctrl+escape"] = "core:quit" }

