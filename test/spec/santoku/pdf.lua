local test = require("santoku.test")
local serialize = require("santoku.serialize") -- luacheck: ignore
local it = require("santoku.iter")
local arr = require("santoku.array")
local pdf = require("santoku.pdf")

test("pdf", function ()

  local els = pdf.walk(os.getenv("TK_PDF_TEST_FILE") or "test/res/bitcoin.pdf")

  -- els = it.filter(function (m, t)
  --   return m == "stream-token" and t == "text"
  -- end, els)

  els = it.take(10000, els)
  it.each(print, els)

  -- print(arr.concat(it.collect(it.map(function (_, _, v)
  --   return v
  -- end, els))))

end)
