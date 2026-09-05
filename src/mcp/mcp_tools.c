/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mcp/mcp_tools.h"
#include "common/capabilities.h"
#include "mcp/dt_bridge.h"

#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// result builders
// ---------------------------------------------------------------------------

// { "content": [ { "type":"text", "text": <text> } ], "isError": <is_error> }
static JsonNode *_text_result(const char *text, gboolean is_error)
{
  JsonObject *block = json_object_new();
  json_object_set_string_member(block, "type", "text");
  json_object_set_string_member(block, "text", text ? text : "");

  JsonArray *content = json_array_new();
  json_array_add_object_element(content, block);

  JsonObject *res = json_object_new();
  json_object_set_array_member(res, "content", content);
  json_object_set_boolean_member(res, "isError", is_error);

  JsonNode *node = json_node_new(JSON_NODE_OBJECT);
  json_node_take_object(node, res);
  return node;
}

static const char *_arg_string(JsonObject *args, const char *key)
{
  if(!args || !json_object_has_member(args, key)) return NULL;
  JsonNode *n = json_object_get_member(args, key);
  if(json_node_get_value_type(n) != G_TYPE_STRING) return NULL;
  return json_node_get_string(n);
}

static int _arg_int(JsonObject *args, const char *key, int fallback)
{
  if(!args || !json_object_has_member(args, key)) return fallback;
  return (int)json_object_get_int_member(args, key);
}

static gboolean _arg_bool(JsonObject *args, const char *key, gboolean fallback)
{
  if(!args || !json_object_has_member(args, key)) return fallback;
  return json_object_get_boolean_member(args, key);
}

// image result: { content:[{ type:image, mimeType:image/png, data:<base64> }] }
static JsonNode *_image_result(const uint8_t *png, size_t len)
{
  gchar *b64 = g_base64_encode(png, len);
  JsonObject *block = json_object_new();
  json_object_set_string_member(block, "type", "image");
  json_object_set_string_member(block, "mimeType", "image/png");
  json_object_set_string_member(block, "data", b64);
  g_free(b64);

  JsonArray *content = json_array_new();
  json_array_add_object_element(content, block);

  JsonObject *res = json_object_new();
  json_object_set_array_member(res, "content", content);
  json_object_set_boolean_member(res, "isError", FALSE);

  JsonNode *node = json_node_new(JSON_NODE_OBJECT);
  json_node_take_object(node, res);
  return node;
}

// shared parsing of the render/image_stats input arguments
static gboolean _parse_render_inputs(JsonObject *args, const char **path,
                                     int *imgid, int *w, int *h, int *he,
                                     gboolean *dtm, JsonArray **stack)
{
  *path = NULL; *imgid = 0; *stack = NULL;
  if(!args) return FALSE;
  if(json_object_has_member(args, "input"))
  {
    JsonNode *in = json_object_get_member(args, "input");
    if(JSON_NODE_HOLDS_OBJECT(in))
    {
      JsonObject *io = json_node_get_object(in);
      if(json_object_has_member(io, "path"))
        *path = json_object_get_string_member(io, "path");
      if(json_object_has_member(io, "imgid"))
        *imgid = (int)json_object_get_int_member(io, "imgid");
    }
  }
  *w = _arg_int(args, "width", 0);
  *h = _arg_int(args, "height", 0);
  *he = _arg_int(args, "history_end", -1);
  *dtm = _arg_bool(args, "disable_tone_mappers", FALSE);
  if(json_object_has_member(args, "stack"))
  {
    JsonNode *s = json_object_get_member(args, "stack");
    if(JSON_NODE_HOLDS_ARRAY(s)) *stack = json_node_get_array(s);
  }
  return (*path != NULL) || (*imgid > 0);
}

// ---------------------------------------------------------------------------
// tool handlers
// ---------------------------------------------------------------------------

static JsonNode *_tool_list_modules(JsonObject *args)
{
  (void)args;
  char *json = dt_bridge_list_modules_json();
  JsonNode *r = _text_result(json, FALSE);
  g_free(json);
  return r;
}

static JsonNode *_tool_module_schema(JsonObject *args)
{
  const char *op = _arg_string(args, "operation");
  if(!op) return _text_result("missing required string argument 'operation'", TRUE);
  char *err = NULL;
  char *json = dt_bridge_module_schema_json(op, &err);
  if(!json)
  {
    JsonNode *r = _text_result(err ? err : "error", TRUE);
    g_free(err);
    return r;
  }
  JsonNode *r = _text_result(json, FALSE);
  g_free(json);
  return r;
}

static JsonNode *_tool_decode_params(JsonObject *args)
{
  const char *op = _arg_string(args, "operation");
  const char *hex = _arg_string(args, "blob_hex");
  if(!op || !hex)
    return _text_result("missing required arguments 'operation' and 'blob_hex'", TRUE);
  char *err = NULL;
  char *json = dt_bridge_decode_params_json(op, hex, &err);
  if(!json)
  {
    JsonNode *r = _text_result(err ? err : "error", TRUE);
    g_free(err);
    return r;
  }
  JsonNode *r = _text_result(json, FALSE);
  g_free(json);
  return r;
}

static JsonNode *_tool_encode_params(JsonObject *args)
{
  const char *op = _arg_string(args, "operation");
  if(!op) return _text_result("missing required string argument 'operation'", TRUE);
  JsonObject *fields = NULL;
  if(args && json_object_has_member(args, "fields"))
  {
    JsonNode *fn = json_object_get_member(args, "fields");
    if(JSON_NODE_HOLDS_OBJECT(fn)) fields = json_node_get_object(fn);
  }
  char *err = NULL;
  char *hex = dt_bridge_encode_params_hex(op, fields, &err);
  if(!hex)
  {
    JsonNode *r = _text_result(err ? err : "error", TRUE);
    g_free(err);
    return r;
  }
  char *out = g_strdup_printf("{\"operation\":\"%s\",\"blob_hex\":\"%s\"}", op, hex);
  JsonNode *r = _text_result(out, FALSE);
  g_free(out);
  g_free(hex);
  return r;
}

static JsonNode *_tool_render(JsonObject *args)
{
  const char *path; int imgid, w, h, he; gboolean dtm; JsonArray *stack;
  if(!_parse_render_inputs(args, &path, &imgid, &w, &h, &he, &dtm, &stack))
    return _text_result("render: provide input.path or input.imgid", TRUE);

  uint8_t *png = NULL;
  size_t len = 0;
  char *err = NULL;
  if(!dt_bridge_render_png(path, imgid, w, h, stack, dtm, he, &png, &len, &err))
  {
    JsonNode *r = _text_result(err ? err : "render failed", TRUE);
    g_free(err);
    return r;
  }
  JsonNode *r = _image_result(png, len);
  g_free(png);
  return r;
}

static JsonNode *_tool_image_stats(JsonObject *args)
{
  const char *path; int imgid, w, h, he; gboolean dtm; JsonArray *stack;
  if(!_parse_render_inputs(args, &path, &imgid, &w, &h, &he, &dtm, &stack))
    return _text_result("image_stats: provide input.path or input.imgid", TRUE);

  char *err = NULL;
  char *json = dt_bridge_image_stats_json(path, imgid, w, h, stack, dtm, he, &err);
  if(!json)
  {
    JsonNode *r = _text_result(err ? err : "image_stats failed", TRUE);
    g_free(err);
    return r;
  }
  JsonNode *r = _text_result(json, FALSE);
  g_free(json);
  return r;
}

static JsonNode *_json_text_or_err(char *json, char *err)
{
  if(!json)
  {
    JsonNode *r = _text_result(err ? err : "error", TRUE);
    g_free(err);
    return r;
  }
  JsonNode *r = _text_result(json, FALSE);
  g_free(json);
  return r;
}

static JsonNode *_tool_list_images(JsonObject *args)
{
  return _json_text_or_err(dt_bridge_list_images_json(_arg_int(args, "limit", 0)), NULL);
}

static JsonNode *_tool_get_history(JsonObject *args)
{
  if(!args || !json_object_has_member(args, "imgid"))
    return _text_result("get_history: requires integer 'imgid'", TRUE);
  char *err = NULL;
  return _json_text_or_err(dt_bridge_get_history_json(_arg_int(args, "imgid", 0), &err),
                           err);
}

static JsonNode *_tool_list_styles(JsonObject *args)
{
  (void)args;
  return _json_text_or_err(dt_bridge_list_styles_json(), NULL);
}

static JsonNode *_tool_apply_style(JsonObject *args)
{
  const char *name = _arg_string(args, "name");
  const int imgid = _arg_int(args, "imgid", 0);
  char *err = NULL;
  if(!dt_bridge_apply_style(name, imgid, _arg_bool(args, "overwrite", FALSE), &err))
  { JsonNode *r = _text_result(err ? err : "error", TRUE); g_free(err); return r; }
  return _text_result("{\"ok\":true}", FALSE);
}

static JsonNode *_tool_save_style(JsonObject *args)
{
  char *err = NULL;
  if(!dt_bridge_save_style(_arg_string(args, "name"), _arg_string(args, "description"),
                           _arg_int(args, "imgid", 0), &err))
  { JsonNode *r = _text_result(err ? err : "error", TRUE); g_free(err); return r; }
  return _text_result("{\"ok\":true}", FALSE);
}

static JsonNode *_tool_import_style(JsonObject *args)
{
  char *err = NULL;
  if(!dt_bridge_import_style(_arg_string(args, "path"), &err))
  { JsonNode *r = _text_result(err ? err : "error", TRUE); g_free(err); return r; }
  return _text_result("{\"ok\":true}", FALSE);
}

static JsonNode *_tool_export(JsonObject *args)
{
  const char *path; int imgid, w, h, he; gboolean dtm; JsonArray *stack;
  _parse_render_inputs(args, &path, &imgid, &w, &h, &he, &dtm, &stack);
  const char *out_path = _arg_string(args, "out_path");
  if((!path && imgid <= 0) || !out_path)
    return _text_result("export: requires input.path/imgid and 'out_path'", TRUE);
  char *err = NULL;
  if(!dt_bridge_export_png(path, imgid, w, h, stack, dtm, he, out_path, &err))
  { JsonNode *r = _text_result(err ? err : "error", TRUE); g_free(err); return r; }
  char *msg = g_strdup_printf("{\"ok\":true,\"out_path\":\"%s\"}", out_path);
  JsonNode *r = _text_result(msg, FALSE);
  g_free(msg);
  return r;
}

static void _add_string_array(JsonObject *object,
                              const char *member,
                              const char *const *values)
{
  JsonArray *array = json_array_new();
  for(const char *const *value = values; value && *value; value++)
    json_array_add_string_element(array, *value);
  json_object_set_array_member(object, member, array);
}

static JsonObject *_capability_object(const dt_capability_descriptor_t *capability)
{
  JsonObject *object = json_object_new();
  json_object_set_string_member(object, "schema_version", "1.0");
  json_object_set_string_member(object, "id", capability->id);
  json_object_set_int_member(object, "version", capability->version);
  json_object_set_string_member(object, "name", capability->name);
  json_object_set_string_member(object, "description", capability->description);
  _add_string_array(object, "synonyms", capability->synonyms);
  _add_string_array(object, "examples", capability->examples);
  json_object_set_string_member(object, "help_reference", capability->help_reference);

  JsonArray *contexts = json_array_new();
  if(capability->contexts & DT_CAPABILITY_CONTEXT_GLOBAL)
    json_array_add_string_element(contexts, "global");
  if(capability->contexts & DT_CAPABILITY_CONTEXT_LIBRARY)
    json_array_add_string_element(contexts, "library");
  if(capability->contexts & DT_CAPABILITY_CONTEXT_EDIT)
    json_array_add_string_element(contexts, "edit");
  if(capability->contexts & DT_CAPABILITY_CONTEXT_EXPORT)
    json_array_add_string_element(contexts, "export");
  json_object_set_array_member(object, "contexts", contexts);

  JsonObject *selection = json_object_new();
  json_object_set_boolean_member(selection, "required", capability->selection_required);
  json_object_set_boolean_member(selection, "multiple", capability->supports_multiple_selection);
  json_object_set_object_member(object, "selection", selection);

  JsonArray *arguments = json_array_new();
  for(guint k = 0; k < capability->argument_count; k++)
  {
    const dt_capability_argument_t *argument = &capability->arguments[k];
    JsonObject *item = json_object_new();
    json_object_set_string_member(item, "name", argument->name);
    json_object_set_string_member(item, "type", dt_capability_argument_type_name(argument->type));
    json_object_set_boolean_member(item, "required", argument->required);
    if(argument->type == DT_CAPABILITY_ARGUMENT_INTEGER
       || argument->type == DT_CAPABILITY_ARGUMENT_NUMBER)
    {
      json_object_set_double_member(item, "minimum", argument->minimum);
      json_object_set_double_member(item, "maximum", argument->maximum);
    }
    if(argument->values)
      _add_string_array(item, "values", argument->values);
    if(argument->default_value)
      json_object_set_string_member(item, "default", argument->default_value);
    if(argument->description)
      json_object_set_string_member(item, "description", argument->description);
    json_array_add_object_element(arguments, item);
  }
  json_object_set_array_member(object, "arguments", arguments);
  json_object_set_string_member(object, "side_effect",
                                dt_capability_side_effect_name(capability->side_effect));

  JsonObject *lifecycle = json_object_new();
  json_object_set_boolean_member(lifecycle, "preview", capability->supports_preview);
  json_object_set_boolean_member(lifecycle, "confirmation", capability->requires_confirmation);
  json_object_set_boolean_member(lifecycle, "undo", capability->supports_undo);
  json_object_set_boolean_member(lifecycle, "cancellation", capability->supports_cancellation);
  json_object_set_object_member(object, "lifecycle", lifecycle);
  json_object_set_string_member(object, "agent_exposure",
                                dt_capability_exposure_name(capability->agent_exposure));
  json_object_set_string_member(object, "navigation_target", capability->navigation_target);
  return object;
}

static char *_json_object_string(JsonObject *object)
{
  JsonNode *node = json_node_new(JSON_NODE_OBJECT);
  json_node_take_object(node, object);
  JsonGenerator *generator = json_generator_new();
  json_generator_set_root(generator, node);
  char *json = json_generator_to_data(generator, NULL);
  g_object_unref(generator);
  json_node_free(node);
  return json;
}

static guint _capability_context(const char *name)
{
  if(!name || !*name) return 0;
  if(!g_strcmp0(name, "global")) return DT_CAPABILITY_CONTEXT_GLOBAL;
  if(!g_strcmp0(name, "library")) return DT_CAPABILITY_CONTEXT_LIBRARY;
  if(!g_strcmp0(name, "edit")) return DT_CAPABILITY_CONTEXT_EDIT;
  if(!g_strcmp0(name, "export")) return DT_CAPABILITY_CONTEXT_EXPORT;
  return G_MAXUINT;
}

static JsonNode *_tool_capabilities_search(JsonObject *args)
{
  const char *query = _arg_string(args, "query");
  const guint context = _capability_context(_arg_string(args, "context"));
  if(context == G_MAXUINT)
    return _text_result("capabilities_search: context must be global, library, edit, or export", TRUE);

  const int requested = CLAMP(_arg_int(args, "limit", 8), 1, 32);
  dt_capability_match_t matches[32] = { 0 };
  const size_t count = dt_capabilities_search(query, context, matches, requested);

  JsonArray *items = json_array_new();
  for(size_t k = 0; k < count; k++)
  {
    JsonObject *item = _capability_object(matches[k].descriptor);
    json_object_set_int_member(item, "score", matches[k].score);
    json_array_add_object_element(items, item);
  }
  JsonObject *root = json_object_new();
  json_object_set_string_member(root, "schema_version", "1.0");
  json_object_set_array_member(root, "capabilities", items);
  g_autofree char *json = _json_object_string(root);
  return _text_result(json, FALSE);
}

static JsonNode *_tool_capabilities_describe(JsonObject *args)
{
  const char *id = _arg_string(args, "id");
  if(!id) return _text_result("capabilities_describe: requires string 'id'", TRUE);
  const dt_capability_descriptor_t *capability = dt_capability_get(id);
  if(!capability) return _text_result("capabilities_describe: unknown capability id", TRUE);
  g_autofree char *json = _json_object_string(_capability_object(capability));
  return _text_result(json, FALSE);
}

// ---------------------------------------------------------------------------
// registry
// ---------------------------------------------------------------------------

typedef JsonNode *(*mcp_tool_fn)(JsonObject *args);

typedef struct mcp_handler_t
{
  const char *name;
  mcp_tool_fn fn;
} mcp_handler_t;

// tool behaviour lives in C; the presentation (name/description/inputSchema)
// is loaded from mcp-tools.json in the data folder and matched here by name
static const mcp_handler_t _handlers[] = {
  { "capabilities_search", _tool_capabilities_search },
  { "capabilities_describe", _tool_capabilities_describe },
  { "list_modules",  _tool_list_modules },
  { "module_schema", _tool_module_schema },
  { "decode_params", _tool_decode_params },
  { "encode_params", _tool_encode_params },
  { "render",        _tool_render },
  { "image_stats",   _tool_image_stats },
  { "list_images",   _tool_list_images },
  { "get_history",   _tool_get_history },
  { "list_styles",   _tool_list_styles },
  { "apply_style",   _tool_apply_style },
  { "save_style",    _tool_save_style },
  { "import_style",  _tool_import_style },
  { "export",        _tool_export },
};
static const size_t _n_handlers = sizeof(_handlers) / sizeof(_handlers[0]);

// tool metadata array (name/description/inputSchema) loaded from mcp-tools.json
static JsonNode *_tools_meta = NULL;

int mcp_tools_load(const char *path)
{
  JsonParser *parser = json_parser_new();
  GError *e = NULL;
  if(!json_parser_load_from_file(parser, path, &e))
  {
    fprintf(stderr, "darktable-mcp: could not load tools file '%s': %s\n",
            path, e ? e->message : "?");
    if(e) g_error_free(e);
    g_object_unref(parser);
    return -1;
  }
  JsonNode *root = json_parser_get_root(parser);
  if(!root || !JSON_NODE_HOLDS_ARRAY(root))
  {
    fprintf(stderr, "darktable-mcp: tools file '%s' is not a JSON array\n", path);
    g_object_unref(parser);
    return -1;
  }
  if(_tools_meta) json_node_unref(_tools_meta);
  _tools_meta = json_node_copy(root);
  g_object_unref(parser);

  // warn about metadata entries with no C handler (they cannot be called)
  JsonArray *arr = json_node_get_array(_tools_meta);
  const guint len = json_array_get_length(arr);
  for(guint k = 0; k < len; k++)
  {
    JsonObject *o = json_array_get_object_element(arr, k);
    const char *name = o && json_object_has_member(o, "name")
                         ? json_object_get_string_member(o, "name") : NULL;
    gboolean known = FALSE;
    for(size_t i = 0; i < _n_handlers; i++)
      if(!g_strcmp0(name, _handlers[i].name)) { known = TRUE; break; }
    if(!known)
      fprintf(stderr, "darktable-mcp: tool '%s' has no handler; not callable\n",
              name ? name : "(unnamed)");
  }

  // warn about handlers missing from the metadata (callable but not advertised)
  for(size_t i = 0; i < _n_handlers; i++)
  {
    gboolean listed = FALSE;
    for(guint k = 0; k < len; k++)
    {
      JsonObject *o = json_array_get_object_element(arr, k);
      const char *name = o && json_object_has_member(o, "name")
                           ? json_object_get_string_member(o, "name") : NULL;
      if(!g_strcmp0(name, _handlers[i].name)) { listed = TRUE; break; }
    }
    if(!listed)
      fprintf(stderr, "darktable-mcp: handler '%s' has no metadata; not listed\n",
              _handlers[i].name);
  }
  return (int)len;
}

JsonNode *mcp_tools_list_node(void)
{
  if(_tools_meta) return json_node_copy(_tools_meta);
  JsonNode *node = json_node_new(JSON_NODE_ARRAY);
  json_node_take_array(node, json_array_new());
  return node;
}

JsonNode *mcp_tools_call_node(const char *name, JsonObject *arguments, gboolean *found)
{
  for(size_t i = 0; i < _n_handlers; i++)
  {
    if(!g_strcmp0(name, _handlers[i].name))
    {
      if(found) *found = TRUE;
      return _handlers[i].fn(arguments);
    }
  }
  if(found) *found = FALSE;
  return NULL;
}
