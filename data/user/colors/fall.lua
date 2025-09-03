local style = require "core.style"
local common = require "core.common"

style.background      = { common.color "#111111" }
style.background2     = { common.color "#0F1315" }
style.background3     = { common.color "#1A1B19" }
style.line_highlight  = { common.color "#2C2D2B" }
style.selection       = { common.color "#49483E" }
style.scrollbar       = { common.color "#3E3D32" }
style.scrollbar2      = { common.color "#70716A" }
style.divider         = { common.color "#3E3D32" }
style.dim             = { common.color "#70716A" }

style.text            = { common.color "#F8F8F2" }
style.caret           = { common.color "#F8F8F0" }
style.accent          = { common.color "#A6E22E" }

style.line_number     = { common.color "#75715E" }
style.line_number2    = { common.color "#A2A093" }

style.syntax["normal"]  = { common.color "#F8F8F2" }
style.syntax["symbol"]  = { common.color "#66D9EF" }
style.syntax["comment"] = { common.color "#75715E" }
style.syntax["keyword"] = { common.color "#F92672" }
style.syntax["keyword2"]= { common.color "#FD971F" }
style.syntax["number"]  = { common.color "#AE81FF" }
style.syntax["literal"] = { common.color "#AE81FF" }
style.syntax["string"]  = { common.color "#E6DB74" }
style.syntax["operator"]= { common.color "#F92672" }
style.syntax["function"]= { common.color "#A6E22E" }