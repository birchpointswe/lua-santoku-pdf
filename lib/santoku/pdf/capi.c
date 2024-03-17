#include "lua.h"
#include "lauxlib.h"

#include "pdfio.h"

#include <string.h>

#define MT_FILE "santoku_pdf_file"
#define MT_PAGE "santoku_pdf_page"
#define MT_DICT "santoku_pdf_dict"
#define MT_ARRAY "santoku_pdf_array"
#define MT_OBJECT "santoku_pdf_object"
#define MT_STREAM "santoku_pdf_stream"

typedef struct {
  lua_State *L;
  pdfio_file_t *file;
} tkpdf_file_t;

typedef struct {
  pdfio_obj_t *page;
} tkpdf_page_t;

typedef struct {
  pdfio_dict_t *dict;
} tkpdf_dict_t;

typedef struct {
  pdfio_array_t *array;
} tkpdf_array_t;

typedef struct {
  pdfio_obj_t *obj;
} tkpdf_obj_t;

typedef struct {
  pdfio_stream_t *stream;
} tkpdf_stream_t;

// TODO: Duplicated across various libraries, need to consolidate
static inline void callmod (lua_State *L, int nargs, int nret, const char *smod, const char *sfn)
{
  lua_getglobal(L, "require"); // arg req
  lua_pushstring(L, smod); // arg req smod
  lua_call(L, 1, 1); // arg mod
  lua_pushstring(L, sfn); // args mod sfn
  lua_gettable(L, -2); // args mod fn
  lua_remove(L, -2); // args fn
  lua_insert(L, - nargs - 1); // fn args
  lua_call(L, nargs, nret); // results
}

#define peek_file(L, i) ((tkpdf_file_t *) luaL_checkudata(L, i, MT_FILE))
#define peek_page(L, i) ((tkpdf_page_t *) luaL_checkudata(L, i, MT_PAGE))
#define peek_dict(L, i) ((tkpdf_dict_t *) luaL_checkudata(L, i, MT_DICT))
#define peek_array(L, i) ((tkpdf_array_t *) luaL_checkudata(L, i, MT_ARRAY))
#define peek_obj(L, i) ((tkpdf_obj_t *) luaL_checkudata(L, i, MT_OBJECT))
#define peek_stream(L, i) ((tkpdf_stream_t *) luaL_checkudata(L, i, MT_STREAM))

static inline bool on_error (pdfio_file_t *, const char *message, void *pdfp)
{
  tkpdf_file_t *tkpdf = (tkpdf_file_t *) pdfp;
  lua_State *L = tkpdf->L;
  lua_pushstring(L, "error parsing pdf");
  lua_pushstring(L, message);
  callmod(L, 3, 0, "santoku.error", "error");
  return 0;
}

static inline int open (lua_State *L)
{
  lua_settop(L, 1); // fp
  tkpdf_file_t *tkpdf = lua_newuserdata(L, sizeof(tkpdf_file_t)); // fp pdf
  luaL_getmetatable(L, MT_FILE); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkpdf->L = L;
  tkpdf->file = pdfioFileOpen(luaL_checkstring(L, 1), NULL, NULL, on_error, tkpdf);
  if (!tkpdf->file)
    luaL_error(L, "error opening pdf");
  return 1;
}

static inline int get_num_pages (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_file_t *tkpdf = peek_file(L, 1);
  lua_pushinteger(L, pdfioFileGetNumPages(tkpdf->file));
  return 1;
}

static inline int get_num_objs (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_file_t *tkpdf = peek_file(L, 1);
  lua_pushinteger(L, pdfioFileGetNumObjs(tkpdf->file));
  return 1;
}

static inline int get_obj (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_file_t *tkpdf = peek_file(L, 1);
  pdfio_obj_t *obj = pdfioFileGetObj(tkpdf->file, luaL_checkinteger(L, 2));
  tkpdf_obj_t *tkobj = lua_newuserdata(L, sizeof(tkpdf_obj_t)); // fp pdf
  luaL_getmetatable(L, MT_OBJECT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkobj->obj = obj;
  return 1;
}

static inline int get_page (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_file_t *tkpdf = peek_file(L, 1);
  pdfio_obj_t *page = pdfioFileGetPage(tkpdf->file, luaL_checkinteger(L, 2));
  if (!page)
    return 0;
  tkpdf_page_t *tkpage = lua_newuserdata(L, sizeof(tkpdf_page_t)); // fp pdf
  luaL_getmetatable(L, MT_PAGE); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkpage->page = page;
  return 1;
}

static inline int get_page_dict (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_page_t *tkpage = peek_page(L, 1);
  pdfio_dict_t *dict = pdfioObjGetDict(tkpage->page);
  if (!dict)
    return 0;
  tkpdf_dict_t *tkdict = lua_newuserdata(L, sizeof(tkpdf_dict_t)); // fp pdf
  luaL_getmetatable(L, MT_DICT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkdict->dict = dict;
  return 1;
}

static bool iter_dict_keys_iterator (pdfio_dict_t *, const char *key, void *Lp)
{
  lua_State *L = (lua_State *) Lp;
  lua_pushvalue(L, 2); // d fn fn
  lua_pushstring(L, key); // d fn fn key
  lua_call(L, 1, 0);
  return true;
}

static inline int iter_dict_keys (lua_State *L)
{
  lua_settop(L, 2); // d fn
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  pdfioDictIterateKeys(tkdict->dict, iter_dict_keys_iterator, L);
  return 0;
}

static inline const char *valtype_name (pdfio_valtype_t t)
{
  switch (t) {
    case PDFIO_VALTYPE_ARRAY:
      return "array";
    case PDFIO_VALTYPE_BINARY:
      return "binary";
    case PDFIO_VALTYPE_BOOLEAN:
      return "boolean";
    case PDFIO_VALTYPE_DATE:
      return "date";
    case PDFIO_VALTYPE_DICT:
      return "dict";
    case PDFIO_VALTYPE_INDIRECT:
      return "object-ref";
    case PDFIO_VALTYPE_NAME:
      return "name";
    case PDFIO_VALTYPE_NONE:
      return "none";
    case PDFIO_VALTYPE_NULL:
      return "null";
    case PDFIO_VALTYPE_NUMBER:
      return "number";
    case PDFIO_VALTYPE_STRING:
      return "string";
    default:
      return NULL;
  }
}

static inline int close (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_file_t *tkpdf = peek_file(L, 1);
  if (tkpdf->file)
    pdfioFileClose(tkpdf->file);
  memset(tkpdf, 0, sizeof(tkpdf_file_t));
  return 0;
}

static inline int get_dict_type (lua_State *L)
{
  lua_settop(L, 2); // d k
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  lua_pushstring(L, valtype_name(pdfioDictGetType(tkdict->dict, luaL_checkstring(L, 2))));
  return 1;
}

static inline int get_dict_dict (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  pdfio_dict_t *dict = pdfioDictGetDict(tkdict->dict, luaL_checkstring(L, 2));
  if (!dict)
    return 0;
  tkpdf_dict_t *tkdict0 = lua_newuserdata(L, sizeof(tkpdf_dict_t)); // fp pdf
  luaL_getmetatable(L, MT_DICT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkdict0->dict = dict;
  return 1;
}

static inline int get_dict_name (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  const char *name = pdfioDictGetName(tkdict->dict, luaL_checkstring(L, 2));
  if (!name)
    return 0;
  lua_pushstring(L, name);
  return 1;
}

static inline int get_dict_string (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  const char *string = pdfioDictGetString(tkdict->dict, luaL_checkstring(L, 2));
  if (!string)
    return 0;
  lua_pushstring(L, string);
  return 1;
}

static inline int get_array_string (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  const char *string = pdfioArrayGetString(tkarray->array, luaL_checkinteger(L, 2));
  if (!string)
    return 0;
  lua_pushstring(L, string);
  return 1;
}

static inline int get_dict_boolean (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  bool b = pdfioDictGetBoolean(tkdict->dict, luaL_checkstring(L, 2));
  lua_pushboolean(L, b);
  return 1;
}

static inline int get_dict_number (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  double d = pdfioDictGetNumber(tkdict->dict, luaL_checkstring(L, 2));
  lua_pushnumber(L, d);
  return 1;
}

static inline int get_dict_date (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  time_t t = pdfioDictGetDate(tkdict->dict, luaL_checkstring(L, 2));
  lua_pushinteger(L, t);
  return 1;
}

static inline int get_dict_binary (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  size_t l;
  unsigned char *a = pdfioDictGetBinary(tkdict->dict, luaL_checkstring(L, 2), &l);
  lua_pushlstring(L, (char *) a, l);
  return 1;
}

static inline int get_dict_array (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  pdfio_array_t *array = pdfioDictGetArray(tkdict->dict, luaL_checkstring(L, 2));
  tkpdf_array_t *tkarray = lua_newuserdata(L, sizeof(tkpdf_array_t)); // fp pdf
  luaL_getmetatable(L, MT_ARRAY); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkarray->array = array;
  return 1;
}

static inline int get_dict_obj (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_dict_t *tkdict = peek_dict(L, 1);
  pdfio_obj_t *obj = pdfioDictGetObj(tkdict->dict, luaL_checkstring(L, 2));
  tkpdf_obj_t *tkobj = lua_newuserdata(L, sizeof(tkpdf_obj_t)); // fp pdf
  luaL_getmetatable(L, MT_OBJECT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkobj->obj = obj;
  return 1;
}

static inline int get_array_type (lua_State *L)
{
  lua_settop(L, 2); // d k
  tkpdf_array_t *tkarray = peek_array(L, 1);
  lua_pushstring(L, valtype_name(pdfioArrayGetType(tkarray->array, luaL_checkinteger(L, 2))));
  return 1;
}

static inline int get_array_dict (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  pdfio_dict_t *dict = pdfioArrayGetDict(tkarray->array, luaL_checkinteger(L, 2));
  if (!dict)
    return 0;
  tkpdf_dict_t *tkdict = lua_newuserdata(L, sizeof(tkpdf_dict_t)); // fp pdf
  luaL_getmetatable(L, MT_DICT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkdict->dict = dict;
  return 1;
}

static inline int get_array_name (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  const char *name = pdfioArrayGetName(tkarray->array, luaL_checkinteger(L, 2));
  if (!name)
    return 0;
  lua_pushstring(L, name);
  return 1;
}

static inline int get_array_boolean (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  bool b = pdfioArrayGetBoolean(tkarray->array, luaL_checkinteger(L, 2));
  lua_pushboolean(L, b);
  return 1;
}

static inline int get_array_number (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  double d = pdfioArrayGetNumber(tkarray->array, luaL_checkinteger(L, 2));
  lua_pushnumber(L, d);
  return 1;
}

static inline int get_array_date (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  time_t t = pdfioArrayGetDate(tkarray->array, luaL_checkinteger(L, 2));
  lua_pushinteger(L, t);
  return 1;
}

static inline int get_array_binary (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  size_t l;
  unsigned char *a = pdfioArrayGetBinary(tkarray->array, luaL_checkinteger(L, 2), &l);
  lua_pushlstring(L, (char *) a, l);
  return 1;
}

static inline int get_array_array (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  pdfio_array_t *array = pdfioArrayGetArray(tkarray->array, luaL_checkinteger(L, 2));
  tkpdf_array_t *tkarray0 = lua_newuserdata(L, sizeof(tkpdf_array_t)); // fp pdf
  luaL_getmetatable(L, MT_ARRAY); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkarray0->array = array;
  return 1;
}

static inline int get_array_obj (lua_State *L)
{
  lua_settop(L, 2);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  pdfio_obj_t *obj = pdfioArrayGetObj(tkarray->array, luaL_checkinteger(L, 2));
  tkpdf_obj_t *tkobj = lua_newuserdata(L, sizeof(tkpdf_obj_t)); // fp pdf
  luaL_getmetatable(L, MT_OBJECT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkobj->obj = obj;
  return 1;
}

static inline int get_array_size (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_array_t *tkarray = peek_array(L, 1);
  size_t s = pdfioArrayGetSize(tkarray->array);
  lua_pushinteger(L, s);
  return 1;
}

static inline int get_obj_type (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_obj_t *tkobj = peek_obj(L, 1);
  const char *t = pdfioObjGetType(tkobj->obj);
  lua_pushstring(L, t);
  return 1;
}

static inline int get_obj_subtype (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_obj_t *tkobj = peek_obj(L, 1);
  const char *t = pdfioObjGetSubtype(tkobj->obj);
  lua_pushstring(L, t);
  return 1;
}

static inline int get_obj_dict (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_obj_t *tkobj = peek_obj(L, 1);
  pdfio_dict_t *dict = pdfioObjGetDict(tkobj->obj);
  if (!dict)
    return 0;
  tkpdf_dict_t *tkdict = lua_newuserdata(L, sizeof(tkpdf_dict_t)); // fp pdf
  luaL_getmetatable(L, MT_DICT); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkdict->dict = dict;
  return 1;
}

static inline int get_obj_array (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_obj_t *tkobj = peek_obj(L, 1);
  pdfio_array_t *array = pdfioObjGetArray(tkobj->obj);
  if (!array)
    return 0;
  tkpdf_array_t *tkarray = lua_newuserdata(L, sizeof(tkpdf_array_t)); // fp pdf
  luaL_getmetatable(L, MT_ARRAY); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkarray->array = array;
  return 1;
}

static inline int get_obj_stream (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_obj_t *tkobj = peek_obj(L, 1);
  pdfio_stream_t *stream = pdfioObjOpenStream(tkobj->obj, true);
  if (!stream)
    return 0;
  tkpdf_stream_t *tkstream = lua_newuserdata(L, sizeof(tkpdf_stream_t)); // fp pdf
  luaL_getmetatable(L, MT_STREAM); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkstream->stream = stream;
  return 1;
}

static inline int get_page_stream (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_obj_t *tkobj = peek_obj(L, 1);
  pdfio_stream_t *stream = pdfioPageOpenStream(tkobj->obj, 0, true);
  if (!stream)
    return 0;
  tkpdf_stream_t *tkstream = lua_newuserdata(L, sizeof(tkpdf_stream_t)); // fp pdf
  luaL_getmetatable(L, MT_STREAM); // fp pdf mt
  lua_setmetatable(L, -2); // fp pdf
  tkstream->stream = stream;
  return 1;
}

static inline int close_stream (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_stream_t *tkstream = peek_stream(L, 1);
  pdfioStreamClose(tkstream->stream);
  return 0;
}

static inline int get_stream_token (lua_State *L)
{
  lua_settop(L, 1);
  tkpdf_stream_t *tkstream = peek_stream(L, 1);
  luaL_Buffer buf;
  luaL_buffinit(L, &buf);
  char *sbuf = luaL_prepbuffer(&buf);
  if (!pdfioStreamGetToken(tkstream->stream, sbuf, LUAL_BUFFERSIZE))
    return 0;
  lua_pushstring(L, "unknown");
  luaL_addsize(&buf, strnlen(sbuf, LUAL_BUFFERSIZE));
  luaL_pushresult(&buf);
  return 2;
}

luaL_Reg fns[] = {
  { "open", open },
  { "close", close },
  { "get_num_pages", get_num_pages },
  { "get_page", get_page },
  { "get_num_objs", get_num_objs },
  { "get_obj", get_obj },
  { "get_page_dict", get_page_dict },
  { "iter_dict_keys", iter_dict_keys },
  { "get_dict_type", get_dict_type },
  { "get_dict_dict", get_dict_dict },
  { "get_dict_name", get_dict_name },
  { "get_dict_boolean", get_dict_boolean },
  { "get_dict_number", get_dict_number },
  { "get_dict_date", get_dict_date },
  { "get_dict_binary", get_dict_binary },
  { "get_dict_array", get_dict_array },
  { "get_dict_obj", get_dict_obj },
  { "get_dict_string", get_dict_string },
  { "get_array_type", get_array_type },
  { "get_array_dict", get_array_dict },
  { "get_array_name", get_array_name },
  { "get_array_boolean", get_array_boolean },
  { "get_array_number", get_array_number },
  { "get_array_date", get_array_date },
  { "get_array_binary", get_array_binary },
  { "get_array_array", get_array_array },
  { "get_array_size", get_array_size },
  { "get_array_obj", get_array_obj },
  { "get_array_string", get_array_string },
  { "get_obj_type", get_obj_type },
  { "get_obj_subtype", get_obj_subtype },
  { "get_obj_dict", get_obj_dict },
  { "get_obj_array", get_obj_array },
  { "get_obj_stream", get_obj_stream },
  { "get_page_stream", get_page_stream },
  { "close_stream", close_stream },
  { "get_stream_token", get_stream_token },
  { NULL, NULL }
};

int luaopen_santoku_pdf_capi (lua_State *L)
{
  lua_newtable(L);
  luaL_register(L, NULL, fns);
  luaL_newmetatable(L, MT_FILE);
  lua_pushcfunction(L, close);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);
  luaL_newmetatable(L, MT_PAGE);
  lua_pop(L, 1);
  luaL_newmetatable(L, MT_DICT);
  lua_pop(L, 1);
  luaL_newmetatable(L, MT_ARRAY);
  lua_pop(L, 1);
  luaL_newmetatable(L, MT_OBJECT);
  lua_pop(L, 1);
  luaL_newmetatable(L, MT_STREAM);
  lua_pop(L, 1);
  return 1;
}
