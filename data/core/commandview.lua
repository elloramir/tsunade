local core = require "core"
local common = require "core.common"
local style = require "core.style"
local Doc = require "core.doc"
local DocView = require "core.docview"
local View = require "core.view"


local SingleLineDoc = Doc:extend()

function SingleLineDoc:insert(line, col, text)
  SingleLineDoc.super.insert(self, line, col, text:gsub("\n", ""))
end


local CommandView = DocView:extend()

local max_suggestions = 10

local noop = function() end

local default_state = {
  submit = noop,
  suggest = noop,
  cancel = noop,
}


function CommandView:new()
  CommandView.super.new(self, SingleLineDoc())
  self.suggestion_idx = 1
  self.suggestions = {}
  self.suggestions_height = 0
  self.last_change_id = 0
  self.gutter_width = 0
  self.gutter_text_brightness = 0
  self.selection_offset = 0
  self.state = default_state
  self.font = "font"
  self.size.y = 0
  self.label = ""
  
  -- New properties for centered display
  self.modal_width = 600  -- Fixed width for the modal
  self.modal_height = 400 -- Maximum height for the modal
  self.input_height = 0   -- Height of the input area
  self.show_backdrop = false
end


function CommandView:get_name()
  return View.get_name(self)
end


function CommandView:get_modal_rect()
  local root_w, root_h = core.root_view.size.x, core.root_view.size.y
  local w = math.min(self.modal_width, root_w - 40)
  local total_h = self.input_height + self.suggestions_height
  local h = math.min(total_h, self.modal_height)
  local x = (root_w - w) / 2
  local y = (root_h - h) / 2 - 50  -- Slightly above center
  
  return x, y, w, h
end


function CommandView:get_input_rect()
  local mx, my, mw, mh = self:get_modal_rect()
  return mx, my, mw, self.input_height
end


function CommandView:get_suggestions_rect()
  local mx, my, mw, mh = self:get_modal_rect()
  local suggestions_h = math.min(self.suggestions_height, mh - self.input_height)
  return mx, my + self.input_height, mw, suggestions_h
end


function CommandView:get_line_screen_position()
  local x, y, w, h = self:get_input_rect()
  local font = self:get_font()
  local label_w = font:get_width(self.label)
  return x + style.padding.x + label_w, y + (h - font:get_height()) / 2
end


function CommandView:get_scrollable_size()
  return 0
end


function CommandView:scroll_to_make_visible()
  -- no-op function to disable this functionality
end


function CommandView:get_text()
  return self.doc:get_text(1, 1, 1, math.huge)
end


function CommandView:set_text(text, select)
  self.doc:remove(1, 1, math.huge, math.huge)
  self.doc:text_input(text)
  if select then
    self.doc:set_selection(math.huge, math.huge, 1, 1)
  end
end


function CommandView:move_suggestion_idx(dir)
  local n = self.suggestion_idx + dir
  self.suggestion_idx = common.clamp(n, 1, #self.suggestions)
  self:complete()
  self.last_change_id = self.doc:get_change_id()
end


function CommandView:complete()
  if #self.suggestions > 0 then
    self:set_text(self.suggestions[self.suggestion_idx].text)
  end
end


function CommandView:submit()
  local suggestion = self.suggestions[self.suggestion_idx]
  local text = self:get_text()
  local submit = self.state.submit
  self:exit(true)
  submit(text, suggestion)
end


function CommandView:enter(text, submit, suggest, cancel)
  if self.state ~= default_state then
    return
  end
  self.state = {
    submit = submit or noop,
    suggest = suggest or noop,
    cancel = cancel or noop,
  }
  core.set_active_view(self)
  self:update_suggestions()
  self.gutter_text_brightness = 100
  self.label = text .. ": "
  self.show_backdrop = true
end


function CommandView:exit(submitted, inexplicit)
  if core.active_view == self then
    core.set_active_view(core.last_active_view)
  end
  local cancel = self.state.cancel
  self.state = default_state
  self.doc:reset()
  self.suggestions = {}
  self.show_backdrop = false
  if not submitted then cancel(not inexplicit) end
end


function CommandView:get_gutter_width()
  return 0  -- No gutter in centered mode
end


function CommandView:get_suggestion_line_height()
  return self:get_font():get_height() + style.padding.y
end


function CommandView:update_suggestions()
  local t = self.state.suggest(self:get_text()) or {}
  local res = {}
  for i, item in ipairs(t) do
    if i == max_suggestions then
      break
    end
    if type(item) == "string" then
      item = { text = item }
    end
    res[i] = item
  end
  self.suggestions = res
  self.suggestion_idx = 1
end


function CommandView:update()
  CommandView.super.update(self)

  if core.active_view ~= self and self.state ~= default_state then
    self:exit(false, true)
  end

  -- update suggestions if text has changed
  if self.last_change_id ~= self.doc:get_change_id() then
    self:update_suggestions()
    self.last_change_id = self.doc:get_change_id()
  end

  -- update gutter text color brightness
  self:move_towards("gutter_text_brightness", 0, 0.1)

  -- update input height
  local dest_input_h = 0
  if self.state ~= default_state then
    dest_input_h = self:get_font():get_height() + style.padding.y * 2
  end
  self:move_towards("input_height", dest_input_h)

  -- update suggestions box height
  local lh = self:get_suggestion_line_height()
  local dest_suggestions_h = #self.suggestions * lh
  self:move_towards("suggestions_height", dest_suggestions_h)

  -- update suggestion cursor offset
  local dest = (self.suggestion_idx - 1) * self:get_suggestion_line_height()
  self:move_towards("selection_offset", dest)
end


function CommandView:draw_line_highlight()
  -- no-op function to disable this functionality
end


function CommandView:draw_line_gutter(idx, x, y)
  -- no-op function - we don't use gutter in centered mode
end


-- Simplified draw_rect for straight rectangles
local function draw_rect_simple(x, y, w, h, color)
  renderer.draw_rect(x, y, w, h, color)
end


local function draw_backdrop(self)
  if not self.show_backdrop then return end
  
  local root_w, root_h = core.root_view.size.x, core.root_view.size.y
  local backdrop_color = { 0, 0, 0, 100 }  -- Semi-transparent black
  renderer.draw_rect(0, 0, root_w, root_h, backdrop_color)
end


local function draw_input_area(self)
  if self.input_height <= 0 then return end
  
  local x, y, w, h = self:get_input_rect()
  
  -- Draw input background
  draw_rect_simple(x, y, w, h, style.background2)
  
  -- Draw input border
  local border_color = style.accent
  renderer.draw_rect(x, y, w, 2, border_color)
  renderer.draw_rect(x, y + h - 2, w, 2, border_color)
  renderer.draw_rect(x, y, 2, h, border_color)
  renderer.draw_rect(x + w - 2, y, 2, h, border_color)
  
  -- Draw label
  local font = self:get_font()
  local text_y = y + (h - font:get_height()) / 2
  local label_x = x + style.padding.x
  renderer.draw_text(font, self.label, label_x, text_y, style.accent)
  
  -- Clip area para o input
  local input_x = label_x + font:get_width(self.label)
  local clip_w = x + w - style.padding.x - input_x
  core.push_clip_rect(input_x, y, clip_w, h)
  
  -- Draw input text
  local input_text = self:get_text()
  renderer.draw_text(font, input_text, input_x, text_y, style.text)
  
  -- Draw cursor
  if core.active_view == self then
    local cursor_x = input_x + font:get_width(input_text)
    renderer.draw_rect(cursor_x, text_y, style.caret_width, font:get_height(), style.caret)
  end
  
  core.pop_clip_rect()
end



local function draw_suggestions_area(self)
  if #self.suggestions == 0 then return end
  
  local sx, sy, sw, sh = self:get_suggestions_rect()
  local lh = self:get_suggestion_line_height()
  
  -- Draw suggestions background (retângulo reto)
  draw_rect_simple(sx, sy, sw, sh, style.background3)
  
  -- Draw separator line
  renderer.draw_rect(sx, sy, sw, 1, style.divider)
  
  -- Draw selection highlight
  local highlight_y = sy + self.selection_offset
  if highlight_y >= sy and highlight_y + lh <= sy + sh then
    renderer.draw_rect(sx, highlight_y, sw, lh, style.line_highlight)
  end
  
  -- Draw suggestions
  core.push_clip_rect(sx, sy, sw, sh)
  for i, item in ipairs(self.suggestions) do
    local item_y = sy + (i - 1) * lh
    if item_y >= sy - lh and item_y <= sy + sh then
      local color = (i == self.suggestion_idx) and style.accent or style.text
      local text_x = sx + style.padding.x
      local text_y = item_y + (lh - self:get_font():get_height()) / 2
      
      renderer.draw_text(self:get_font(), item.text, text_x, text_y, color)
      
      if item.info then
        local info_w = sw - text_x - style.padding.x
        common.draw_text(self:get_font(), style.dim, item.info, "right", text_x, text_y, info_w, lh)
      end
    end
  end
  core.pop_clip_rect()
end


function CommandView:draw()
  if self.state == default_state then return end
  
  -- Use defer_draw to ensure we draw on top of everything
  core.root_view:defer_draw(function()
    draw_backdrop(self)
    draw_input_area(self)
    draw_suggestions_area(self)
  end)
end


return CommandView
