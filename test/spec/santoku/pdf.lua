local test = require("santoku.test")
local pdf = require("santoku.pdf")

test("pdf capi: open and count objects", function ()
  local file = pdf.open("test/res/bitcoin.pdf")
  assert(pdf.get_num_objs(file) > 0)
  pdf.close(file)
end)

test("pdf walk: drives events and yields open-obj", function ()
  local step = pdf.walk("test/res/bitcoin.pdf")
  local n = 0
  local saw_open_obj = false
  while n < 20000 do
    local ev = step()
    if ev == nil then
      break
    end
    if ev == "open-obj" then
      saw_open_obj = true
    end
    n = n + 1
  end
  assert(n > 0)
  assert(saw_open_obj)
end)
