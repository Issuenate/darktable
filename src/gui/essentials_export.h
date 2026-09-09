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

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum dt_essentials_export_result_t
{
  DT_ESSENTIALS_EXPORT_CANCELLED = 0,
  DT_ESSENTIALS_EXPORT_QUEUED,
  /* the user asked for the complete export module: every format darktable can
   * write, plus profiles, styles, metadata and filename patterns */
  DT_ESSENTIALS_EXPORT_WANTS_FULL_MODULE,
} dt_essentials_export_result_t;

/*
 * The Essentials export: four questions -- where, what format, how big, how
 * good -- and a button. It writes the same configuration keys the full export
 * module uses and hands the work to dt_control_export(), so there is no second
 * export path to keep in step; opening the full module afterwards shows exactly
 * what was chosen here.
 */
dt_essentials_export_result_t dt_essentials_export_dialog(void);

/*
 * The folder half of a disk-storage filename pattern, for the folder chooser.
 * plugins/imageio/storage/disk/file_directory is a pattern, not a folder --
 * the default is "$(FILE_FOLDER)/darktable_exported/$(FILE_NAME)" -- so the
 * last component has to come off. A plain path with no variable in it is
 * already a folder. Exposed so the parsing can be tested; see
 * src/tests/unittests/test_essentials_export.c.
 */
gchar *dt_essentials_export_directory_from_pattern(const char *pattern);

G_END_DECLS

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
