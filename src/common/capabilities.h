/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#pragma once

#include <glib.h>
#include <stddef.h>

typedef enum dt_capability_context_t
{
  DT_CAPABILITY_CONTEXT_GLOBAL = 1 << 0,
  DT_CAPABILITY_CONTEXT_LIBRARY = 1 << 1,
  DT_CAPABILITY_CONTEXT_EDIT = 1 << 2,
  DT_CAPABILITY_CONTEXT_EXPORT = 1 << 3
} dt_capability_context_t;

typedef enum dt_capability_side_effect_t
{
  DT_CAPABILITY_READ_ONLY,
  DT_CAPABILITY_REVERSIBLE_EDIT,
  DT_CAPABILITY_LIBRARY_MUTATION,
  DT_CAPABILITY_FILE_OUTPUT,
  DT_CAPABILITY_DESTRUCTIVE
} dt_capability_side_effect_t;

typedef enum dt_capability_exposure_t
{
  DT_CAPABILITY_INTERNAL_ONLY,
  DT_CAPABILITY_DISCOVERY,
  DT_CAPABILITY_AGENT_READ_ONLY,
  DT_CAPABILITY_PREVIEWABLE,
  DT_CAPABILITY_EXECUTABLE
} dt_capability_exposure_t;

typedef enum dt_capability_argument_type_t
{
  DT_CAPABILITY_ARGUMENT_BOOLEAN,
  DT_CAPABILITY_ARGUMENT_INTEGER,
  DT_CAPABILITY_ARGUMENT_NUMBER,
  DT_CAPABILITY_ARGUMENT_STRING,
  DT_CAPABILITY_ARGUMENT_ENUM
} dt_capability_argument_type_t;

typedef struct dt_capability_argument_t
{
  const char *name;
  dt_capability_argument_type_t type;
  gboolean required;
  double minimum;
  double maximum;
  const char *default_value;
  const char *description;
  /* NULL-terminated accepted values, required for DT_CAPABILITY_ARGUMENT_ENUM */
  const char *const *values;
} dt_capability_argument_t;

typedef struct dt_capability_descriptor_t
{
  const char *id;
  guint version;
  const char *name;
  const char *description;
  const char *const *synonyms;
  const char *const *examples;
  const char *help_reference;
  guint contexts;
  gboolean selection_required;
  const dt_capability_argument_t *arguments;
  guint argument_count;
  dt_capability_side_effect_t side_effect;
  gboolean supports_preview;
  gboolean requires_confirmation;
  gboolean supports_undo;
  gboolean supports_cancellation;
  gboolean supports_multiple_selection;
  dt_capability_exposure_t agent_exposure;
  const char *navigation_target;
} dt_capability_descriptor_t;

typedef struct dt_capability_match_t
{
  const dt_capability_descriptor_t *descriptor;
  int score;
} dt_capability_match_t;

/* Marks a literal capability dependency in Essentials UI code for CI checks. */
#define DT_ESSENTIALS_ACTION(id) (id)

/*
 * Essentials albums are darktable tags below one reserved hierarchy, so an
 * album stays visible, editable and exportable from the Advanced interface and
 * survives in the XMP sidecar. The trailing '*' selects a whole tag hierarchy
 * in a collection rule; see get_query_string() in common/collection.c:1677.
 */
#define DT_ESSENTIALS_ALBUM_ROOT "album"
#define DT_ESSENTIALS_ALBUM_PREFIX DT_ESSENTIALS_ALBUM_ROOT "|"
#define DT_ESSENTIALS_ALBUM_FILTER DT_ESSENTIALS_ALBUM_ROOT "*"

/*
 * Whether the guided Essentials interface is active. "auto" means the choice
 * has not been made yet: a new user starts guided, an existing one does not.
 * The first caller writes the resolved value back, so the answer cannot change
 * underneath the UI once anything has asked.
 */
gboolean dt_essentials_mode_is_active(void);

size_t dt_capabilities_count(void);
const dt_capability_descriptor_t *dt_capability_at(size_t index);
const dt_capability_descriptor_t *dt_capability_get(const char *id);

/*
 * Returns up to max_matches deterministic, highest-scoring matches. A zero
 * context searches all capabilities. The returned descriptors are static.
 */
size_t dt_capabilities_search(const char *query, guint context,
                              dt_capability_match_t *matches,
                              size_t max_matches);

const char *dt_capability_side_effect_name(dt_capability_side_effect_t value);
const char *dt_capability_exposure_name(dt_capability_exposure_t value);
const char *
dt_capability_argument_type_name(dt_capability_argument_type_t value);
