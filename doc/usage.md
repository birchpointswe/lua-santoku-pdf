# santoku-pdf usage

Worked examples for the two layers: the `walk` event stream and the raw typed getters.
All snippets read structure only; nothing here renders, writes, or extracts page text.
For the PDF object model and pdfio's value types, see the
[pdfio documentation](https://github.com/michaelrsweet/pdfio).

## The walk event loop

`walk(path)` returns a stateless-looking iterator function. Each call returns one event
name plus extra values, or `nil` when the document is exhausted. The iterator opens the
file on creation and closes it when it runs out of objects.

```lua
local pdf = require("santoku.pdf")

local step = pdf.walk("test/res/bitcoin.pdf")

local depth = 0
local function indent () return string.rep("  ", depth) end

for event, a, b, c in step do
  if event == "open-obj" then
    print(indent() .. "obj type=" .. tostring(a) .. " subtype=" .. tostring(b))
    depth = depth + 1
  elseif event == "close-obj" then
    depth = depth - 1
  elseif event == "open-dict" then
    print(indent() .. "{")
    depth = depth + 1
  elseif event == "close-dict" then
    depth = depth - 1
    print(indent() .. "}")
  elseif event == "open-array" then
    print(indent() .. "[")
    depth = depth + 1
  elseif event == "close-array" then
    depth = depth - 1
    print(indent() .. "]")
  elseif event == "key" then
    -- a = name, b = value type, c = value (scalars only)
    print(indent() .. a .. " : " .. b .. (c ~= nil and (" = " .. tostring(c)) or ""))
  elseif event == "index" then
    -- a = position (1-based), b = value type, c = value (scalars only)
    print(indent() .. "[" .. a .. "] : " .. b .. (c ~= nil and (" = " .. tostring(c)) or ""))
  end
end
```

Event reference:

| Event | Extra values | Meaning |
|-------|--------------|---------|
| `open-obj` / `close-obj` | `type`, `subtype` | enter / leave a top-level or referenced object |
| `open-dict` / `close-dict` | none | enter / leave a dictionary |
| `key` | `name`, `type`, `value` | a dictionary entry; `value` is set only for scalar types |
| `open-array` / `close-array` | none | enter / leave an array |
| `index` | `position`, `type`, `value` | an array element; `position` is 1-based, `value` for scalars only |

Behavior to keep in mind:

- For a `key` or `index` whose `type` is `dict`, `array`, or `object-ref`, the value is not
  returned inline; the walker pushes that child and emits its own `open-*`/`close-*` events
  next. Scalar types (`name`, `string`, `boolean`, `number`, `date`) carry their value in
  the event.
- A dict key named `Parent` is skipped to avoid following the page-tree back-reference and
  cycling. Other `object-ref` values are followed.
- Each object's dict (if any) and array (if any) are both descended.

Anchor: `test/spec/santoku/pdf.lua`.

## Filtering for a value during a walk

The event stream is flat, so collecting a specific value is a match on `key` events. This
pulls the document title if a catalog or info dictionary carries one.

```lua
local pdf = require("santoku.pdf")

local step = pdf.walk("doc.pdf")
for event, name, ty, value in step do
  if event == "key" and name == "Title" and ty == "string" then
    print("title:", value)
  end
end
```

## Raw getters: open, count, fetch

The C core is available through the same module. Open a file, read its object count, fetch
objects by index (1-based here), then pull each object's dict or array.

```lua
local pdf = require("santoku.pdf")

local file = pdf.open("doc.pdf")
local n = pdf.get_num_objs(file)

for i = 1, n do
  local obj = pdf.get_obj(file, i)
  if obj then
    local ty = pdf.get_obj_type(obj)       -- pdfio's object type string
    local st = pdf.get_obj_subtype(obj)    -- subtype string, or nil
    print(i, ty, st)
  end
end

pdf.close(file)
```

## Raw getters: typed dict and array reads

Read the type first, then call the matching getter. Array indices are 0-based.

```lua
local pdf = require("santoku.pdf")

local file = pdf.open("doc.pdf")
local obj = pdf.get_obj(file, 1)
local dict = pdf.get_obj_dict(obj)

if dict then
  pdf.iter_dict_keys(dict, function (key)
    local ty = pdf.get_dict_type(dict, key)
    if ty == "name" then
      print(key, "name", pdf.get_dict_name(dict, key))
    elseif ty == "number" then
      print(key, "number", pdf.get_dict_number(dict, key))
    elseif ty == "array" then
      local array = pdf.get_dict_array(dict, key)
      local size = pdf.get_array_size(array)
      for idx = 0, size - 1 do            -- 0-based
        print(key, idx, pdf.get_array_type(array, idx))
      end
    end
  end)
end

pdf.close(file)
```

The dict getters are `get_dict_{type,dict,array,name,boolean,number,date,binary,obj,string}`
plus `iter_dict_keys`; the array getters are `get_array_{type,dict,array,name,boolean,number,
date,binary,obj,size,string}`. `get_dict_obj` and `get_array_obj` return the referenced
object userdata for an `object-ref` value, which you then read with `get_obj_*`. See
`lib/santoku/pdf/capi.c` for the full `luaL_Reg` list.

## Page-level entry points

pdfio also exposes pages directly. These are not used by the walker but are available on the
module: `get_num_pages(file)`, `get_page(file, i)`, and `get_page_dict(page)`. A page's dict
reads with the same `get_dict_*` getters as any other dict.

```lua
local pdf = require("santoku.pdf")

local file = pdf.open("doc.pdf")
for i = 1, pdf.get_num_pages(file) do
  local page = pdf.get_page(file, i)
  local dict = page and pdf.get_page_dict(page)
  if dict then
    print(i, pdf.get_dict_type(dict, "MediaBox"))
  end
end
pdf.close(file)
```

## Gotchas

- Mixed index bases: `get_obj` and `get_page` are 1-based; every `get_array_*` is 0-based.
  In the walker, `index` positions are reported 1-based.
- `date` values come back as integer timestamps, not formatted strings.
- A typed getter returns `nil` when the key or element is missing or of a different type, so
  branch on `get_dict_type`/`get_array_type` first.
- The walker follows `object-ref` values; without the `Parent` skip a page tree would cycle.
  If you add your own raw-getter traversal, guard against cycles yourself.
- The file handle closes on `__gc`, but call `close(file)` to release it promptly when you
  hold many open files.
