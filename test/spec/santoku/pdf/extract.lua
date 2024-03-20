local test = require("santoku.test")
local serialize = require("santoku.serialize") -- luacheck: ignore
local pdf = require("santoku.pdf")

test("extract", function ()
  local els = pdf.extract("test/res/bitcoin.pdf")
  print(serialize(els))
end)
