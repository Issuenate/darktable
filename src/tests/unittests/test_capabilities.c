/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "common/capabilities.h"

#include <cmocka.h>

static void test_ids_are_stable_and_unique(void **state)
{
  (void)state;
  assert_true(dt_capabilities_count() > 0);
  for(size_t i = 0; i < dt_capabilities_count(); i++)
  {
    const dt_capability_descriptor_t *capability = dt_capability_at(i);
    assert_non_null(capability);
    assert_non_null(capability->id);
    assert_true(capability->version > 0);
    assert_ptr_equal(capability, dt_capability_get(capability->id));
    for(size_t j = i + 1; j < dt_capabilities_count(); j++)
      assert_string_not_equal(capability->id, dt_capability_at(j)->id);
  }
}

static void test_natural_language_discovery(void **state)
{
  (void)state;
  dt_capability_match_t matches[4] = {0};

  assert_true(dt_capabilities_search("make brighter", 0, matches, 4) > 0);
  assert_string_equal(matches[0].descriptor->id, "edit.exposure.adjust");

  assert_true(dt_capabilities_search("fix horizon", 0, matches, 4) > 0);
  assert_string_equal(matches[0].descriptor->id, "edit.geometry.straighten");

  assert_true(dt_capabilities_search("reduce noise", 0, matches, 4) > 0);
  assert_string_equal(matches[0].descriptor->id, "edit.detail.denoise");

  assert_true(dt_capabilities_search("export for Instagram", 0, matches, 4) >
              0);
  assert_string_equal(matches[0].descriptor->id, "export.open");

  assert_true(dt_capabilities_search("edit photos from my usb drive", 0,
                                     matches, 4) > 0);
  assert_string_equal(matches[0].descriptor->id, "library.add_photos");
}

static void test_context_filter_and_safety_metadata(void **state)
{
  (void)state;
  dt_capability_match_t matches[8] = {0};
  const size_t count = dt_capabilities_search("", DT_CAPABILITY_CONTEXT_LIBRARY,
                                              matches, G_N_ELEMENTS(matches));
  assert_true(count > 0);
  for(size_t i = 0; i < count; i++)
    assert_true(matches[i].descriptor->contexts &
                DT_CAPABILITY_CONTEXT_LIBRARY);

  const dt_capability_descriptor_t *rating = dt_capability_get("library.rate");
  assert_non_null(rating);
  assert_true(rating->requires_confirmation);
  assert_true(rating->supports_preview);
  assert_true(rating->supports_undo);
  assert_int_equal(rating->side_effect, DT_CAPABILITY_LIBRARY_MUTATION);
  assert_int_equal(rating->arguments[0].minimum, 0);
  assert_int_equal(rating->arguments[0].maximum, 5);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_ids_are_stable_and_unique),
      cmocka_unit_test(test_natural_language_discovery),
      cmocka_unit_test(test_context_filter_and_safety_metadata)};
  return cmocka_run_group_tests(tests, NULL, NULL);
}
