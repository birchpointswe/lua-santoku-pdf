local err = require("santoku.error")
local error = err.error

local arr = require("santoku.array")
local apush = arr.push

local cjson = require("cjson")
local jsdecode = cjson.decode

local tbl = require("santoku.table")
local merge = tbl.merge

local sys = require("santoku.system")
local execute = sys.execute

local fs = require("santoku.fs")
local rm = fs.rm
local readfile = fs.readfile
local tmpname = fs.tmpname

local pdf = require("santoku.pdf.capi")

local open = pdf.open
local close = pdf.close

local get_num_objs = pdf.get_num_objs
local get_obj = pdf.get_obj

local get_dict_type = pdf.get_dict_type
local get_dict_dict = pdf.get_dict_dict
local get_dict_array = pdf.get_dict_array
local get_dict_name = pdf.get_dict_name
local get_dict_boolean = pdf.get_dict_boolean
local get_dict_number = pdf.get_dict_number
local get_dict_date = pdf.get_dict_date
local get_dict_obj = pdf.get_dict_obj
local get_dict_string = pdf.get_dict_string
local iter_dict_keys = pdf.iter_dict_keys

local get_array_size = pdf.get_array_size
local get_array_type = pdf.get_array_type
local get_array_dict = pdf.get_array_dict
local get_array_array = pdf.get_array_array
local get_array_name = pdf.get_array_name
local get_array_boolean = pdf.get_array_boolean
local get_array_number = pdf.get_array_number
local get_array_date = pdf.get_array_date
local get_array_obj = pdf.get_array_obj
local get_array_string = pdf.get_array_string

local get_obj_type = pdf.get_obj_type
local get_obj_subtype = pdf.get_obj_subtype
local get_obj_dict = pdf.get_obj_dict
local get_obj_array = pdf.get_obj_array

local function step (stack)

  local el = stack[#stack]

  if not el then
    return
  end

  if el.file and not el.objs then
    el.objs = get_num_objs(el.file)
    el.current = 0
    if el.objs == 0 then
      close(el.file)
      stack[#stack] = nil
      return
    else
      return step(stack)
    end
  end

  if el.file and el.objs then
    el.current = el.current + 1
    if el.current > el.objs then
      close(el.file)
      stack[#stack] = nil
      return
    else
      local obj = get_obj(el.file, el.current)
      if not obj then
        return step(stack)
      else
        local t = get_obj_type(obj)
        local st = get_obj_subtype(obj)
        apush(stack, { obj = obj, n = el.current, type = t, subtype = st })
        return step(stack)
      end
    end
  end

  if el.dict and not el.keys then
    el.keys = {}
    el.current = 0
    iter_dict_keys(el.dict, function (k)
      apush(el.keys, k)
    end)
    return "open-dict"
  end

  if el.dict and el.keys then
    el.current = el.current + 1
    if el.current > #el.keys then
      stack[#stack] = nil
      return "close-dict"
    end
    local name = el.keys[el.current]
    local typ = get_dict_type(el.dict, name)
    local v
    if typ == "dict" then
      local dict = get_dict_dict(el.dict, name)
      apush(stack, { dict = dict })
    elseif typ == "array" then
      local array = get_dict_array(el.dict, name)
      apush(stack, { array = array })
    elseif typ == "object-ref" and name ~= "Parent" then
      local obj = get_dict_obj(el.dict, name)
      local t = get_obj_type(obj)
      local st = get_obj_subtype(obj)
      apush(stack, { obj = obj, type = t, subtype = st })
    elseif typ == "name" then
      v = get_dict_name(el.dict, name)
    elseif typ == "string" then
      v = get_dict_string(el.dict, name)
    elseif typ == "boolean" then
      v = get_dict_boolean(el.dict, name)
    elseif typ == "number" then
      v = get_dict_number(el.dict, name)
    elseif typ == "date" then
      v = get_dict_date(el.dict, name)
    end
    return "key", name, typ, v
  end

  if el.array and not el.max then
    el.max = get_array_size(el.array)
    el.current = 0
    return "open-array"
  end

  if el.array and el.max then
    el.current = el.current + 1
    if el.current > el.max then
      stack[#stack] = nil
      return "close-array"
    end
    local typ = get_array_type(el.array, el.current - 1)
    local v
    if typ == "dict" then
      local dict = get_array_dict(el.array, el.current - 1)
      apush(stack, { dict = dict })
    elseif typ == "array" then
      local array = get_array_array(el.array, el.current - 1)
      apush(stack, { array = array })
    elseif typ == "object-ref" then
      local obj = get_array_obj(el.array, el.current - 1)
      apush(stack, { obj = obj, type = get_obj_type(obj), subtype = get_obj_subtype(obj) })
    elseif typ == "name" then
      v = get_array_name(el.array, el.current - 1)
    elseif typ == "string" then
      v = get_array_string(el.array, el.current - 1)
    elseif typ == "boolean" then
      v = get_array_boolean(el.array, el.current - 1)
    elseif typ == "number" then
      v = get_array_number(el.array, el.current - 1)
    elseif typ == "date" then
      v = get_array_date(el.array, el.current - 1)
    end
    return "index", el.current, typ, v
  end

  if el.obj and not el.visited then
    el.visited = true
    local x
    x = get_obj_dict(el.obj)
    if x then
      apush(stack, { dict = x })
    end
    x = get_obj_array(el.obj)
    if x then
      apush(stack, { array = x })
    end
    return "open-obj", el.type, el.subtype
  end

  if el.obj and el.visited then
    stack[#stack] = nil
    return "close-obj", el.type, el.subtype
  end

  error("invalid parser state", #stack)

end

local function walk (fp)
  local stack = { { file = open(fp) } }
  return function (...)
    return step(stack, ...)
  end
end

local function extract (fp)
  local tfp = tmpname()
  execute({
    "mutool", "convert",
    "-F", "stext.json",
    "-O", "preserve-images,dehyphenate",
    "-o", tfp, fp
  })
  local res = jsdecode(readfile(tfp))
  rm(tfp)
  return res
end

return merge({
  walk = walk,
  extract = extract,
}, pdf)
