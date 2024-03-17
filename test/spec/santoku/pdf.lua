local test = require("santoku.test")
local serialize = require("santoku.serialize") -- luacheck: ignore
local it = require("santoku.iter")
local pdf = require("santoku.pdf")

test("pdf", function ()
  it.each(print, it.take(100, pdf.walk("test/res/bitcoin.pdf")))
end)
