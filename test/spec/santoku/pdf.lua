local test = require("santoku.test")
local serialize = require("santoku.serialize") -- luacheck: ignore
local it = require("santoku.iter")
local pdf = require("santoku.pdf")

test("pdf", function ()
  local els = pdf.walk("test/res/bitcoin.pdf")
  els = it.take(10000, els)
  it.each(print, els)
end)
