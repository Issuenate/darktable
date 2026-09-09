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

#include "gui/essentials_export.h"

#include <cmocka.h>
#include <glib.h>
#include <string.h>

/*
 * plugins/imageio/storage/disk/file_directory is a filename pattern, not a
 * folder. Storing a bare folder there made disk.c treat the whole path as the
 * output file's name, writing Exports.jpg and then Exports_01.jpg rather than
 * putting anything inside the folder. These pin the parsing that reads a
 * folder back out of such a pattern for the chooser.
 */
static void test_pattern_yields_its_folder(void **state)
{
  (void)state;
  gchar *got = dt_essentials_export_directory_from_pattern(
      "/Users/x/Pictures/Exports/$(FILE_NAME)");
  assert_string_equal(got, "/Users/x/Pictures/Exports");
  g_free(got);
}

static void test_plain_path_is_already_a_folder(void **state)
{
  (void)state;
  /* an earlier build of the dialog stored a bare folder here, and a user may
   * type one into the full export module. Taking its dirname would offer the
   * parent, not the folder they chose. */
  gchar *got = dt_essentials_export_directory_from_pattern("/Users/x/Pictures/Exports");
  assert_string_equal(got, "/Users/x/Pictures/Exports");
  g_free(got);
}

static void test_variable_folder_falls_back(void **state)
{
  (void)state;
  /* darktable's own default. Its folder half is itself a variable, which no
   * file chooser can show, so it must not be offered as a path. */
  gchar *got = dt_essentials_export_directory_from_pattern(
      "$(FILE_FOLDER)/darktable_exported/$(FILE_NAME)");
  assert_non_null(got);
  assert_null(strstr(got, "$("));
  assert_true(g_path_is_absolute(got));
  g_free(got);
}

static void test_empty_and_null_fall_back(void **state)
{
  (void)state;
  for(const char *input = ""; input; input = NULL)
  {
    gchar *got = dt_essentials_export_directory_from_pattern(input);
    assert_non_null(got);
    assert_true(g_path_is_absolute(got));
    g_free(got);
    if(!input) break;
  }
  gchar *from_null = dt_essentials_export_directory_from_pattern(NULL);
  assert_non_null(from_null);
  assert_true(g_path_is_absolute(from_null));
  g_free(from_null);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_pattern_yields_its_folder),
      cmocka_unit_test(test_plain_path_is_already_a_folder),
      cmocka_unit_test(test_variable_folder_falls_back),
      cmocka_unit_test(test_empty_and_null_fall_back)};
  return cmocka_run_group_tests(tests, NULL, NULL);
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
