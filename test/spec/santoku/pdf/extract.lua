local test = require("santoku.test")
local serialize = require("santoku.serialize") -- luacheck: ignore
local it = require("santoku.iter")
local arr = require("santoku.array")
local pdf = require("santoku.pdf")

test("pdf", function ()
  local t = pdf.extract(os.getenv("TK_PDF_TEST_FILE") or "test/res/bitcoin.pdf", 2)
  for i = 1, #t do
    local text = t[i]
    print("text", text.xs, text.ys, text.x, text.y)
    for j = 1, #text.chunks do
      local chunk = text.chunks[j]
      print("  chunk", chunk.x, chunk.y, chunk.n, chunk.text)
    end
    print()
  end
end)
