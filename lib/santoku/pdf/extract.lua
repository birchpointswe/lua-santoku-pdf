-- TODO: Look up glyph widths when calculating change of x
-- TODO: Fix Author et. al formatting (it gets pushed up)

local pdf = require("santoku.pdf.capi")
local arr = require("santoku.array")
local str = require("santoku.string")
local err = require("santoku.error")

local function extract (fp, max)

  local els = {}
  local file = pdf.open(fp)
  local pages = pdf.get_num_pages(file)

  local toks = {}
  local text = nil

  for i = 1, pages do

    local page = pdf.get_page(file, i - 1)
    local stream = pdf.get_page_stream(page)

    while true do

      local tok = pdf.get_stream_token(stream)

      print("tok", tok)

      if not tok or (max and #els >= max)  then
        pdf.close_stream(stream)
        break
      end

      arr.push(toks, tok)

      if tok == "BT" then
        text = { chunks = {} }
      elseif tok == "ET" then
        arr.sort(text.chunks, function (a, b)
          if a.y == b.y and a.x == b.x then
            return a.n < b.n
          elseif a.y == b.y then
            return a.x < b.x
          elseif a.y ~= b.y then
            return a.y > b.y
          else
            return a.n < b.n
          end
        end)
        arr.push(els, text)
      elseif tok == "Td" or tok == "TD" then
        local x, y = tonumber(toks[#toks - 2]), tonumber(toks[#toks - 1])
        arr.clear(toks)
        if not text.x then
          text.xs = x
          text.ys = y
          text.x = x
          text.y = y
        else
          text.x = text.xs + #text.chunks[#text.chunks].text
          text.y = text.y - 1 + y
        end
      elseif tok == "T*" then
        text.y = text.y - 1
      elseif tok == "Tj" then
        local s = toks[#toks - 1]
        arr.push(text.chunks, { n = #text.chunks, x = text.x, y = text.y, text = s })
        -- text.x = text.x - 1
        -- text.x = text.x - #s
        text.x = text.x + #s
      elseif tok == "TJ" then
        local first
        for i = #toks - 2, 1, -1 do
          if toks[i] == "[" then
            first = i + 1
            break
          end
        end
        for i = first, #toks - 2 do
          if str.match(toks[i], "^%(") then
            local s = str.sub(toks[i], 2)
            arr.push(text.chunks, { n = #text.chunks, x = text.x, y = text.y, text = s })
            -- text.x = text.x - 1
            -- text.x = text.x - #s
            text.x = text.x + #s
          elseif str.match(toks[i], "^%<") then
            -- TODO: decode hex string
            local s = str.sub(toks[i], 2)
            arr.push(text.chunks, { n = #text.chunks, x = text.x, y = text.y, text = s })
            -- text.x = text.x - 1
            -- text.x = text.x - #s
            text.x = text.x + #s
          else
            text.x = text.x - tonumber(toks[i]) / 1000
          end
        end
        arr.clear(toks)
      elseif tok == "'" or tok == '"' then
        text.y = text.y - 1
        local s = toks[#toks - 1]
        arr.push(text.chunks, { n = #text.chunks, x = text.x, y = text.y, text = s })
        -- text.x = text.x + 1
        -- text.x = text.x - #s
        text.x = text.x + #s
      end

    end

  end

  return els

end

return { extract = extract }
