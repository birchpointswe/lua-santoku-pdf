# Santoku PDF

A Lua library for parsing and walking PDF document structures using the pdfio C library.

Santoku PDF provides low-level access to PDF document internals, allowing you to iterate through PDF objects, dictionaries, and arrays. This module is designed for extracting structural information from PDF files rather than rendering or modifying them.

## Module Reference

### `santoku.pdf`

PDF document parsing and structure iteration.

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `walk` | `filepath` | `iterator` | Returns an iterator that walks through PDF structure |
| `open` | `filepath` | `file` | Opens a PDF file for parsing |
| `close` | `file` | `nil` | Closes an opened PDF file |
| `get_num_objs` | `file` | `number` | Returns the number of objects in the PDF |
| `get_obj` | `file, index` | `object/nil` | Gets an object by index |
| `get_obj_type` | `object` | `string` | Returns the type of an object |
| `get_obj_subtype` | `object` | `string/nil` | Returns the subtype of an object |
| `get_obj_dict` | `object` | `dict/nil` | Gets the dictionary from an object |
| `get_obj_array` | `object` | `array/nil` | Gets the array from an object |

### Dictionary Functions

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `get_dict_type` | `dict, key` | `string` | Returns the type of a dictionary value |
| `get_dict_dict` | `dict, key` | `dict/nil` | Gets a nested dictionary |
| `get_dict_array` | `dict, key` | `array/nil` | Gets an array from dictionary |
| `get_dict_name` | `dict, key` | `string/nil` | Gets a name value |
| `get_dict_boolean` | `dict, key` | `boolean/nil` | Gets a boolean value |
| `get_dict_number` | `dict, key` | `number/nil` | Gets a numeric value |
| `get_dict_date` | `dict, key` | `string/nil` | Gets a date value |
| `get_dict_obj` | `dict, key` | `object/nil` | Gets an object reference |
| `get_dict_string` | `dict, key` | `string/nil` | Gets a string value |
| `iter_dict_keys` | `dict, fn` | `nil` | Iterates over dictionary keys |

### Array Functions

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `get_array_size` | `array` | `number` | Returns the size of an array |
| `get_array_type` | `array, index` | `string` | Returns the type at index (0-based) |
| `get_array_dict` | `array, index` | `dict/nil` | Gets a dictionary at index |
| `get_array_array` | `array, index` | `array/nil` | Gets a nested array at index |
| `get_array_name` | `array, index` | `string/nil` | Gets a name value at index |
| `get_array_boolean` | `array, index` | `boolean/nil` | Gets a boolean at index |
| `get_array_number` | `array, index` | `number/nil` | Gets a number at index |
| `get_array_date` | `array, index` | `string/nil` | Gets a date at index |
| `get_array_obj` | `array, index` | `object/nil` | Gets an object reference at index |
| `get_array_string` | `array, index` | `string/nil` | Gets a string at index |

## Walker Event Types

The `walk` function returns an iterator that yields various events as it traverses the PDF structure:

- `"open-obj", type, subtype` - Opening a PDF object
- `"close-obj", type, subtype` - Closing a PDF object
- `"open-dict"` - Opening a dictionary
- `"close-dict"` - Closing a dictionary
- `"key", name, type, value` - Dictionary key-value pair
- `"open-array"` - Opening an array
- `"close-array"` - Closing an array
- `"index", position, type, value` - Array element

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