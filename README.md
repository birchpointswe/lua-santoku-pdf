# santoku-pdf

Read-only access to the structure of a PDF file: its objects, dictionaries, and
arrays. Built on `santoku` (base) and the [pdfio](https://github.com/michaelrsweet/pdfio)
C library (Michael R. Sweet), vendored and built at v1.2.0, linked against libpdfio and
zlib.

This is a structure parser. It does not render pages, modify or write documents, or
extract page text. It reads the object graph that pdfio exposes and presents it to Lua,
either through raw typed getters or through a single event-stream walker.

This README is a usage guide, not an API reference. **The tests are the spec**: the one
test file exercises the walker against a fixture PDF. Read it for the surface; read this
(and [`doc/usage.md`](doc/usage.md)) for how the two layers are used.

For PDF object-model semantics (objects, indirect references, dictionaries, name/string/
date value types), see the pdfio documentation. This library does not re-describe the PDF
format; it maps pdfio's value types onto Lua strings (see the type tags below).

## Modules

| Module | Role | Anchor test |
|--------|------|-------------|
| `santoku.pdf` | re-exports the C core and adds `walk(path)`, the event-stream iterator | `test/spec/santoku/pdf.lua` |
| `santoku.pdf.capi` | the C core: open/close a file, fetch objects, and read typed dict/array values | `test/spec/santoku/pdf.lua` |

## Two layers

The C core (`santoku.pdf.capi`) gives typed getters over four userdata kinds (file, object,
dict, array). You open a file, ask it for its object count, fetch objects by index, and from
each object pull its dictionary or array. Dict and array values are read with type-specific
getters: you first call `get_dict_type`/`get_array_type` to learn the value type, then call
the matching getter (`get_dict_name`, `get_array_number`, and so on). Array indices are
0-based. The file userdata closes itself on garbage collection via `__gc`.

The Lua wrapper (`santoku.pdf`) re-exports every C function and adds `walk(path)`, which
drives the whole object graph as a flat event stream so you do not manage the userdata or
the recursion yourself.

## Value type tags

`get_dict_type` and `get_array_type` return one of these strings (mapped from pdfio's value
types): `array`, `binary`, `boolean`, `date`, `dict`, `object-ref` (an indirect reference),
`name`, `none`, `null`, `number`, `string`. The matching typed getter returns a Lua value;
`date` is returned as an integer timestamp.

## The walker

```lua
local pdf = require("santoku.pdf")

local step = pdf.walk("test/res/bitcoin.pdf")
for event, a, b in step do
  if event == "open-obj" then
    print("object", a, b)        -- a = type, b = subtype
  elseif event == "key" then
    print("key", a, b)           -- a = name, b = value type (c = value if scalar)
  elseif event == "index" then
    print("index", a, b)         -- a = position (1-based), b = value type
  end
end
```

`walk` opens the file, iterates every object, and descends into each object's dict and
array. It emits these events (with their extra return values):

- `open-obj`, `close-obj`: `type`, `subtype`
- `open-dict`, `close-dict`: none
- `key`: `name`, `type`, `value` (value present only for scalar types)
- `open-array`, `close-array`: none
- `index`: `position` (1-based), `type`, `value` (value present only for scalar types)

For `dict`, `array`, and `object-ref` values the walker descends, so the value arrives as a
nested `open-*`/`close-*` pair rather than inline. To avoid reference cycles it skips a dict
key named `Parent` (a `Parent` indirect reference is not followed). The file is closed
automatically when the walk runs out of objects.

Anchor: `test/spec/santoku/pdf.lua` (a `walk` smoke run over the fixture, plus assertions
that `get_num_objs` is positive and that `walk` yields an `open-obj` event).

## Raw getters

```lua
local pdf = require("santoku.pdf")

local file = pdf.open("doc.pdf")
local n = pdf.get_num_objs(file)
local obj = pdf.get_obj(file, 1)             -- objects are 1-based here
local dict = pdf.get_obj_dict(obj)           -- nil if the object has no dict
if dict and pdf.get_dict_type(dict, "Type") == "name" then
  print(pdf.get_dict_name(dict, "Type"))
end
pdf.close(file)
```

The C core also exposes page-level entry points (`get_num_pages`, `get_page`,
`get_page_dict`) and binary value getters (`get_dict_binary`, `get_array_binary`) that the
walker does not use. The walker covers the dict/array/object getters end to end; see the
test for the exercised surface and `lib/santoku/pdf/capi.c` for the full `luaL_Reg` list.

## Conventions

- Object indices through `get_obj` are 1-based; array indices through `get_array_*` are
  0-based (they pass straight to pdfio).
- Typed getters return `nil` when the key or element is absent or of another type; check
  the type first with `get_dict_type`/`get_array_type`.
- The file userdata owns the pdfio handle and closes it on `__gc`; `close(file)` is
  available for explicit release and is what the walker calls when it finishes.

## Building and testing

This repo uses the `toku` build harness. The pdfio C library is vendored and built from
source (`deps/pdfio/Makefile.tk`, pinned to v1.2.0); the build downloads, configures, and
compiles it, then links libpdfio and zlib into the extension. Run the suite through `toku`
so the native module is compiled and on the path. The test depends on `santoku` and
[`santoku-fs`](../lua-santoku-fs/README.md); see [`santoku`](../lua-santoku/README.md) for
the base utilities used in the wrapper.

## License

MIT License

Copyright 2025 Birch Point SWE

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
