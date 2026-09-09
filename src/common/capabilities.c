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

#include "common/capabilities.h"

#include <string.h>

static const char *const _add_photos_synonyms[] = {
    "import", "import photos", "bring in photos", "add images", "usb",
    "external drive", "open from disk", NULL};
static const char *const _add_photos_examples[] = {
    "add photos from this folder", "browse photos on my usb drive",
    "edit photos from an external disk", NULL};
static const char *const _all_photos_synonyms[] = {
    "everything", "entire library", "show all", NULL};
static const char *const _all_photos_examples[] = {
    "show all photos", "go back to my whole library", NULL};
static const char *const _recent_synonyms[] = {
    "latest", "new photos", "last import", "recent import", NULL};
static const char *const _recent_examples[] = {"show what I just imported",
                                               "find my latest photos", NULL};
static const char *const _date_synonyms[] = {"calendar", "taken on", "year",
                                             "month",    "day",      NULL};
static const char *const _date_examples[] = {"photos from last May",
                                             "show photos taken in 2024", NULL};
static const char *const _people_synonyms[] = {"person", "faces", "family",
                                               "friends", NULL};
static const char *const _people_examples[] = {"find photos of people", NULL};
static const char *const _places_synonyms[] = {"map", "location", "where",
                                               "gps", NULL};
static const char *const _places_examples[] = {
    "find photos from Berlin", "show photos near this place", NULL};
static const char *const _tags_synonyms[] = {"keywords", "labels", "topics",
                                             NULL};
static const char *const _tags_examples[] = {"find photos tagged landscape",
                                             NULL};
static const char *const _rating_synonyms[] = {"stars", "favorite", "pick",
                                               "rate", NULL};
static const char *const _rating_examples[] = {"give these photos four stars",
                                               "mark this as a favorite", NULL};
static const char *const _reject_synonyms[] = {"remove from picks", "bad photo",
                                               "discard", NULL};
static const char *const _reject_examples[] = {"reject these photos", NULL};
static const char *const _album_synonyms[] = {"collection", "group photos",
                                              "organize", NULL};
static const char *const _album_examples[] = {
    "add these to my landscapes album", NULL};
static const char *const _albums_synonyms[] = {"album", "my albums",
                                               "collections", NULL};
static const char *const _albums_examples[] = {"show my albums",
                                               "open the landscapes album",
                                               NULL};
static const char *const _label_synonyms[] = {"color label", "colour label",
                                              "flag", "mark", "red", "yellow",
                                              "green", "blue", "purple", NULL};
static const char *const _label_examples[] = {"mark these photos red",
                                              "give this a green label", NULL};
static const char *const _open_edit_synonyms[] = {"develop", "edit photo",
                                                  "start editing", NULL};
static const char *const _open_edit_examples[] = {"open this photo for editing",
                                                  NULL};
static const char *const _export_synonyms[] = {
    "save jpeg", "share", "instagram", "social export", NULL};
static const char *const _export_examples[] = {"export for Instagram",
                                               "save a full-size JPEG", NULL};
static const char *const _advanced_synonyms[] = {
    "classic darktable", "technical tools", "modules", NULL};
static const char *const _advanced_examples[] = {"show advanced tools", NULL};
static const char *const _exposure_synonyms[] = {
    "make brighter", "make darker", "brightness", "lighten", "darken", NULL};
static const char *const _exposure_examples[] = {
    "make this half a stop brighter", "reduce exposure by 0.3 EV", NULL};
static const char *const _horizon_synonyms[] = {"fix horizon", "straighten",
                                                "level", "rotate", NULL};
static const char *const _horizon_examples[] = {"fix the horizon",
                                                "straighten this photo", NULL};
static const char *const _noise_synonyms[] = {"reduce noise", "denoise",
                                              "grain", "high ISO", NULL};
static const char *const _noise_examples[] = {"reduce noise in this photo",
                                              NULL};
static const char *const _contrast_synonyms[] = {"add contrast", "flat photo",
                                                  "more punch", NULL};
static const char *const _contrast_examples[] = {"add a little contrast", NULL};
static const char *const _white_balance_synonyms[] = {
    "temperature", "warmer", "cooler", "remove color cast", NULL};
static const char *const _white_balance_examples[] = {
    "make this photo warmer", "cool down the white balance", NULL};
static const char *const _brilliance_synonyms[] = {
    "brilliance", "tonal contrast", "clarity", NULL};
static const char *const _brilliance_examples[] = {
    "add brilliance without clipping", NULL};
static const char *const _vibrance_synonyms[] = {
    "vibrance", "richer colors", "muted colors", NULL};
static const char *const _vibrance_examples[] = {"make the colors richer", NULL};
static const char *const _saturation_synonyms[] = {
    "saturation", "color intensity", "desaturate", NULL};
static const char *const _saturation_examples[] = {
    "reduce the color intensity", NULL};
static const char *const _tone_curve_synonyms[] = {
    "tone curve", "curve", "tone mapping", "filmic", "sigmoid",
    "dynamic range", "recover the sky", NULL};
static const char *const _tone_curve_examples[] = {
    "adjust the tone curve", "bring back the blown out sky", NULL};
static const char *const _grading_synonyms[] = {
    "color grading",  "color balance", "split toning", "lift gamma gain",
    "shadows tint",   "midtones tint", "highlights tint", NULL};
static const char *const _grading_examples[] = {
    "grade the shadows cooler", "split tone this photo",
    "add a teal tint to the shadows", NULL};
static const char *const _mixer_synonyms[] = {
    "color mixer", "color equalizer", "hue shift", "per-color adjustment",
    "make the sky bluer", NULL};
static const char *const _mixer_examples[] = {
    "make only the greens more saturated", "shift the blues towards cyan",
    NULL};
static const char *const _detail_synonyms[] = {
    "detail", "texture", "sharpen", "crisp", NULL};
static const char *const _detail_examples[] = {"bring out more detail", NULL};
static const char *const _optics_synonyms[] = {
    "lens correction", "distortion", "vignette correction", NULL};
static const char *const _optics_examples[] = {
    "correct the lens distortion", NULL};
static const char *const _profile_synonyms[] = {
    "camera profile", "color profile", "rendering profile", NULL};
static const char *const _profile_examples[] = {
    "change the color profile", NULL};
static const char *const _monochrome_synonyms[] = {
    "black and white", "b&w", "monochrome", NULL};
static const char *const _monochrome_examples[] = {
    "make this black and white", NULL};
static const char *const _highlights_synonyms[] = {
    "bright areas", "recover highlights", "sky detail", NULL};
static const char *const _highlights_examples[] = {
    "recover detail in the highlights", NULL};
static const char *const _shadows_synonyms[] = {
    "dark areas", "lift shadows", "shadow detail", NULL};
static const char *const _shadows_examples[] = {
    "open up the shadows", NULL};
static const char *const _whites_synonyms[] = {
    "white point", "brightest whites", NULL};
static const char *const _whites_examples[] = {
    "make the whites brighter", NULL};
static const char *const _blacks_synonyms[] = {
    "black point", "deep blacks", NULL};
static const char *const _blacks_examples[] = {
    "deepen the blacks", NULL};
static const char *const _texture_synonyms[] = {
    "texture", "microcontrast", "surface detail", NULL};
static const char *const _texture_examples[] = {
    "add texture to the subject", NULL};
static const char *const _clarity_synonyms[] = {
    "clarity", "local contrast", "presence", NULL};
static const char *const _clarity_examples[] = {
    "add a little clarity", NULL};
static const char *const _dehaze_synonyms[] = {
    "dehaze", "remove haze", "fog", NULL};
static const char *const _dehaze_examples[] = {
    "remove haze from the landscape", NULL};
static const char *const _vignette_synonyms[] = {
    "vignette", "darken edges", "edge fade", NULL};
static const char *const _vignette_examples[] = {
    "add a subtle vignette", NULL};
static const char *const _grain_synonyms[] = {
    "film grain", "analog grain", "texture noise", NULL};
static const char *const _grain_examples[] = {
    "add subtle film grain", NULL};
static const char *const _crop_synonyms[] = {
    "crop", "reframe", "aspect ratio", NULL};
static const char *const _crop_examples[] = {
    "crop this to a square", NULL};
static const char *const _sharpen_synonyms[] = {
    "sharpen", "sharpness", "crisp edges", NULL};
static const char *const _sharpen_examples[] = {
    "sharpen this photo", NULL};

static const dt_capability_argument_t _rating_arguments[] = {
    {"stars", DT_CAPABILITY_ARGUMENT_INTEGER, TRUE, 0.0, 5.0, "0",
     "star rating"}};
static const dt_capability_argument_t _album_arguments[] = {
    {"album", DT_CAPABILITY_ARGUMENT_STRING, TRUE, 0.0, 0.0, NULL,
     "album name"}};
static const char *const _label_argument_values[] = {
    "red", "yellow", "green", "blue", "purple", "none", NULL};
static const dt_capability_argument_t _label_arguments[] = {
    {"color", DT_CAPABILITY_ARGUMENT_ENUM, TRUE, 0.0, 0.0, NULL, "color label",
     _label_argument_values}};
static const dt_capability_argument_t _exposure_arguments[] = {
    {"ev", DT_CAPABILITY_ARGUMENT_NUMBER, TRUE, -5.0, 5.0, "0",
     "exposure change in EV"}};
static const dt_capability_argument_t _percent_arguments[] = {
    {"amount", DT_CAPABILITY_ARGUMENT_NUMBER, TRUE, -100.0, 100.0, "0",
     "friendly adjustment amount"}};
static const dt_capability_argument_t _temperature_arguments[] = {
    {"kelvin", DT_CAPABILITY_ARGUMENT_NUMBER, TRUE, 2000.0, 50000.0, "6500",
     "color temperature in kelvin"}};
static const dt_capability_argument_t _positive_percent_arguments[] = {
    {"amount", DT_CAPABILITY_ARGUMENT_NUMBER, TRUE, 0.0, 100.0, "0",
     "friendly adjustment amount"}};

/* keeps an argument array and its count from drifting apart */
#define DT_CAPABILITY_ARGS(a) \
  .arguments = (a), .argument_count = G_N_ELEMENTS(a)

static const dt_capability_descriptor_t _capabilities[] = {
  { .id = "library.add_photos", .version = 1,
    .name = "add photos",
    .description = "browse a usb drive or folder and edit photos in place",
    .synonyms = _add_photos_synonyms, .examples = _add_photos_examples,
    .help_reference = "import",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_LIBRARY_MUTATION,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = TRUE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.header.add_photos" },

  { .id = "library.all_photos", .version = 1,
    .name = "all photos",
    .description = "show every photo in the library",
    .synonyms = _all_photos_synonyms, .examples = _all_photos_examples,
    .help_reference = "collect",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_AGENT_READ_ONLY,
    .navigation_target = "essentials.library.all_photos" },

  { .id = "library.recently_added", .version = 1,
    .name = "recently added",
    .description = "show the latest imports",
    .synonyms = _recent_synonyms, .examples = _recent_examples,
    .help_reference = "collect",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_AGENT_READ_ONLY,
    .navigation_target = "essentials.library.recently_added" },

  { .id = "library.by_date", .version = 1,
    .name = "by date",
    .description = "find photos by capture date",
    .synonyms = _date_synonyms, .examples = _date_examples,
    .help_reference = "collect",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_AGENT_READ_ONLY,
    .navigation_target = "essentials.library.by_date" },

  { .id = "library.albums", .version = 1,
    .name = "albums",
    .description = "show photos organized in albums",
    .synonyms = _albums_synonyms, .examples = _albums_examples,
    .help_reference = "collect",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_AGENT_READ_ONLY,
    .navigation_target = "essentials.library.albums" },

  { .id = "library.people", .version = 1,
    .name = "people",
    .description = "find photos using person tags",
    .synonyms = _people_synonyms, .examples = _people_examples,
    .help_reference = "collect",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_DISCOVERY,
    .navigation_target = "essentials.library.people" },

  { .id = "library.places", .version = 1,
    .name = "places",
    .description = "find geotagged photos by place",
    .synonyms = _places_synonyms, .examples = _places_examples,
    .help_reference = "map",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_DISCOVERY,
    .navigation_target = "essentials.library.places" },

  { .id = "library.tags", .version = 1,
    .name = "tags",
    .description = "find photos by keyword",
    .synonyms = _tags_synonyms, .examples = _tags_examples,
    .help_reference = "tagging",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = FALSE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_AGENT_READ_ONLY,
    .navigation_target = "essentials.library.tags" },

  { .id = "library.rate", .version = 1,
    .name = "rate photos",
    .description = "set a star rating",
    .synonyms = _rating_synonyms, .examples = _rating_examples,
    .help_reference = "ratings",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_rating_arguments),
    .side_effect = DT_CAPABILITY_LIBRARY_MUTATION,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.inspector.rating" },

  { .id = "library.reject", .version = 1,
    .name = "reject photos",
    .description = "mark photos as rejected",
    .synonyms = _reject_synonyms, .examples = _reject_examples,
    .help_reference = "ratings",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_LIBRARY_MUTATION,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.inspector.reject" },

  { .id = "library.label", .version = 1,
    .name = "color label",
    .description = "mark photos with a color label",
    .synonyms = _label_synonyms, .examples = _label_examples,
    .help_reference = "colorlabels",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_label_arguments),
    .side_effect = DT_CAPABILITY_LIBRARY_MUTATION,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.inspector.label" },

  { .id = "library.add_to_album", .version = 1,
    .name = "add to album",
    .description = "organize photos in an album",
    .synonyms = _album_synonyms, .examples = _album_examples,
    .help_reference = "collect",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_album_arguments),
    .side_effect = DT_CAPABILITY_LIBRARY_MUTATION,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.inspector.album" },

  { .id = "edit.open", .version = 1,
    .name = "open in edit",
    .description = "open the selected photo in the editor",
    .synonyms = _open_edit_synonyms, .examples = _open_edit_examples,
    .help_reference = "darkroom",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY,
    .selection_required = TRUE,
    .supports_multiple_selection = FALSE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_AGENT_READ_ONLY,
    .navigation_target = "essentials.inspector.open_edit" },

  { .id = "export.open", .version = 1,
    .name = "export photos",
    .description = "prepare copies for saving or sharing",
    .synonyms = _export_synonyms, .examples = _export_examples,
    .help_reference = "export",
    .contexts = DT_CAPABILITY_CONTEXT_LIBRARY | DT_CAPABILITY_CONTEXT_EXPORT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_FILE_OUTPUT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = FALSE,
    .supports_cancellation = TRUE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.inspector.export" },

  /* a signpost, not a switch: the complete interface is reached through the
     preference, so this says where it lives rather than turning it on */
  { .id = "ui.advanced", .version = 1,
    .name = "advanced mode",
    .description = "where to turn on the complete darktable interface",
    .synonyms = _advanced_synonyms, .examples = _advanced_examples,
    .help_reference = "preferences",
    .contexts = DT_CAPABILITY_CONTEXT_GLOBAL,
    .selection_required = FALSE,
    .supports_multiple_selection = FALSE,
    .side_effect = DT_CAPABILITY_READ_ONLY,
    .supports_preview = FALSE,
    .requires_confirmation = FALSE,
    .supports_undo = FALSE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_DISCOVERY,
    .navigation_target = "preferences.miscellaneous.interface" },

  { .id = "edit.profile.choose", .version = 1,
    .name = "profile",
    .description = "choose the input color profile",
    .synonyms = _profile_synonyms, .examples = _profile_examples,
    .help_reference = "colorin",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.profile" },

  { .id = "edit.monochrome.toggle", .version = 1,
    .name = "black & white",
    .description = "convert a photo to black and white",
    .synonyms = _monochrome_synonyms, .examples = _monochrome_examples,
    .help_reference = "monochrome",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.profile.monochrome" },

  { .id = "edit.exposure.adjust", .version = 1,
    .name = "exposure",
    .description = "make a photo brighter or darker",
    .synonyms = _exposure_synonyms, .examples = _exposure_examples,
    .help_reference = "exposure",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_exposure_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.exposure" },

  { .id = "edit.contrast.adjust", .version = 1,
    .name = "contrast",
    .description = "increase or reduce tonal contrast",
    .synonyms = _contrast_synonyms, .examples = _contrast_examples,
    .help_reference = "colorbalancergb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.contrast" },

  { .id = "edit.highlights.adjust", .version = 1,
    .name = "highlights",
    .description = "adjust the brightest tonal regions",
    .synonyms = _highlights_synonyms, .examples = _highlights_examples,
    .help_reference = "toneequal",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.highlights" },

  { .id = "edit.shadows.adjust", .version = 1,
    .name = "shadows",
    .description = "adjust detail in dark tonal regions",
    .synonyms = _shadows_synonyms, .examples = _shadows_examples,
    .help_reference = "toneequal",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.shadows" },

  { .id = "edit.whites.adjust", .version = 1,
    .name = "whites",
    .description = "adjust the brightest white tones",
    .synonyms = _whites_synonyms, .examples = _whites_examples,
    .help_reference = "toneequal",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.whites" },

  { .id = "edit.blacks.adjust", .version = 1,
    .name = "blacks",
    .description = "adjust the darkest black tones",
    .synonyms = _blacks_synonyms, .examples = _blacks_examples,
    .help_reference = "toneequal",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.blacks" },

  { .id = "edit.white_balance.adjust", .version = 1,
    .name = "white balance",
    .description = "make a photo warmer or cooler",
    .synonyms = _white_balance_synonyms, .examples = _white_balance_examples,
    .help_reference = "channelmixerrgb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_temperature_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.color.white_balance" },

  { .id = "edit.light.brilliance", .version = 1,
    .name = "brilliance",
    .description = "adjust global brightness and tonal presence",
    .synonyms = _brilliance_synonyms, .examples = _brilliance_examples,
    .help_reference = "colorbalancergb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.brilliance" },

  { .id = "edit.color.vibrance", .version = 1,
    .name = "vibrance",
    .description = "adjust muted colors more than saturated colors",
    .synonyms = _vibrance_synonyms, .examples = _vibrance_examples,
    .help_reference = "colorbalancergb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.color.vibrance" },

  { .id = "edit.color.saturation", .version = 1,
    .name = "saturation",
    .description = "adjust overall color intensity",
    .synonyms = _saturation_synonyms, .examples = _saturation_examples,
    .help_reference = "colorbalancergb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.color.saturation" },

  { .id = "edit.light.tone_curve", .version = 1,
    .name = "tone curve",
    .description = "shape how the full range of light is mapped to the photo",
    .synonyms = _tone_curve_synonyms, .examples = _tone_curve_examples,
    .help_reference = "filmicrgb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.light.tone_curve" },

  { .id = "edit.color.grading", .version = 1,
    .name = "color grading",
    .description = "tint the shadows, midtones and highlights separately",
    .synonyms = _grading_synonyms, .examples = _grading_examples,
    .help_reference = "colorbalancergb",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.color.grading" },

  { .id = "edit.color.mixer", .version = 1,
    .name = "color mixer",
    .description = "adjust hue, saturation and brightness one color at a time",
    .synonyms = _mixer_synonyms, .examples = _mixer_examples,
    .help_reference = "colorequal",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.color.mixer" },

  { .id = "edit.effects.texture", .version = 1,
    .name = "texture",
    .description = "adjust fine local contrast",
    .synonyms = _texture_synonyms, .examples = _texture_examples,
    .help_reference = "contrastntexture",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.effects.texture" },

  { .id = "edit.effects.clarity", .version = 1,
    .name = "clarity",
    .description = "adjust medium-scale local contrast",
    .synonyms = _clarity_synonyms, .examples = _clarity_examples,
    .help_reference = "bilat",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.effects.clarity" },

  { .id = "edit.effects.dehaze", .version = 1,
    .name = "dehaze",
    .description = "reduce or add atmospheric haze",
    .synonyms = _dehaze_synonyms, .examples = _dehaze_examples,
    .help_reference = "hazeremoval",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.effects.dehaze" },

  { .id = "edit.effects.vignette", .version = 1,
    .name = "vignette",
    .description = "brighten or darken the photo edges",
    .synonyms = _vignette_synonyms, .examples = _vignette_examples,
    .help_reference = "vignette",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.effects.vignette" },

  { .id = "edit.effects.grain", .version = 1,
    .name = "grain",
    .description = "add a film-like grain texture",
    .synonyms = _grain_synonyms, .examples = _grain_examples,
    .help_reference = "grain",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_positive_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.effects.grain" },

  { .id = "edit.geometry.straighten", .version = 1,
    .name = "straighten",
    .description = "level a tilted photo",
    .synonyms = _horizon_synonyms, .examples = _horizon_examples,
    .help_reference = "rotate_and_perspective",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.geometry.straighten" },

  { .id = "edit.geometry.crop", .version = 1,
    .name = "crop",
    .description = "crop or reframe a photo",
    .synonyms = _crop_synonyms, .examples = _crop_examples,
    .help_reference = "crop",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.geometry.crop" },

  { .id = "edit.detail.denoise", .version = 1,
    .name = "reduce noise",
    .description = "reduce digital image noise",
    .synonyms = _noise_synonyms, .examples = _noise_examples,
    .help_reference = "denoiseprofile",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.detail.denoise" },

  { .id = "edit.detail.adjust", .version = 1,
    .name = "detail",
    .description = "bring out or soften fine detail",
    .synonyms = _detail_synonyms, .examples = _detail_examples,
    .help_reference = "bilat",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_positive_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.detail.amount" },

  { .id = "edit.detail.sharpen", .version = 1,
    .name = "sharpening",
    .description = "increase edge sharpness",
    .synonyms = _sharpen_synonyms, .examples = _sharpen_examples,
    .help_reference = "sharpen",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    DT_CAPABILITY_ARGS(_positive_percent_arguments),
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.detail.sharpening" },

  { .id = "edit.optics.correct", .version = 1,
    .name = "lens corrections",
    .description = "correct common lens distortion and shading",
    .synonyms = _optics_synonyms, .examples = _optics_examples,
    .help_reference = "lens",
    .contexts = DT_CAPABILITY_CONTEXT_EDIT,
    .selection_required = TRUE,
    .supports_multiple_selection = TRUE,
    .side_effect = DT_CAPABILITY_REVERSIBLE_EDIT,
    .supports_preview = TRUE,
    .requires_confirmation = TRUE,
    .supports_undo = TRUE,
    .supports_cancellation = FALSE,
    .agent_exposure = DT_CAPABILITY_PREVIEWABLE,
    .navigation_target = "essentials.edit.optics.corrections" }
};

#undef DT_CAPABILITY_ARGS

size_t dt_capabilities_count(void) { return G_N_ELEMENTS(_capabilities); }

const dt_capability_descriptor_t *dt_capability_at(const size_t index)
{
  return index < G_N_ELEMENTS(_capabilities) ? &_capabilities[index] : NULL;
}

const dt_capability_descriptor_t *dt_capability_get(const char *id)
{
  if(!id)
    return NULL;
  for(size_t k = 0; k < G_N_ELEMENTS(_capabilities); k++)
    if(!g_strcmp0(id, _capabilities[k].id))
      return &_capabilities[k];
  return NULL;
}

static gboolean _contains(const char *haystack, const char *needle)
{
  if(!haystack || !needle || !*needle)
    return FALSE;
  g_autofree gchar *folded_haystack = g_utf8_casefold(haystack, -1);
  return strstr(folded_haystack, needle) != NULL;
}

static int _score_text(const char *text, const char *query, gchar **tokens)
{
  if(!text)
    return 0;
  g_autofree gchar *folded = g_utf8_casefold(text, -1);
  if(!g_strcmp0(folded, query))
    return 900;
  int score = strstr(folded, query) ? 500 : 0;
  for(gchar **token = tokens; token && *token; token++)
  {
    if(!**token)
      continue;
    if(g_str_has_prefix(folded, *token))
      score += 90;
    else if(strstr(folded, *token))
      score += 45;
  }
  return score;
}

static int _score_capability(const dt_capability_descriptor_t *capability,
                             const char *query, gchar **tokens)
{
  g_autofree gchar *folded_id = g_utf8_casefold(capability->id, -1);
  if(!g_strcmp0(folded_id, query))
    return 10000;

  int score = _score_text(capability->name, query, tokens) * 5 +
              _score_text(capability->description, query, tokens);
  for(const char *const *text = capability->synonyms; text && *text; text++)
    score += _score_text(*text, query, tokens) * 3;
  for(const char *const *text = capability->examples; text && *text; text++)
    score += _score_text(*text, query, tokens) * 2;

  for(gchar **token = tokens; token && *token; token++)
    if(**token && !_contains(capability->name, *token) &&
        !_contains(capability->description, *token))
    {
      gboolean found = FALSE;
      for(const char *const *text = capability->synonyms;
           text && *text && !found; text++)
        found = _contains(*text, *token);
      for(const char *const *text = capability->examples;
           text && *text && !found; text++)
        found = _contains(*text, *token);
      if(!found)
        score -= 120;
    }
  return score;
}

static gint _compare_matches(gconstpointer left, gconstpointer right)
{
  const dt_capability_match_t *a = left;
  const dt_capability_match_t *b = right;
  if(a->score != b->score)
    return b->score - a->score;
  return g_strcmp0(a->descriptor->id, b->descriptor->id);
}

size_t dt_capabilities_search(const char *query, const guint context,
                              dt_capability_match_t *matches,
                              const size_t max_matches)
{
  if(!matches || max_matches == 0)
    return 0;
  g_autofree gchar *folded = g_utf8_casefold(query ? query : "", -1);
  g_strstrip(folded);
  g_auto(GStrv) tokens = *folded ? g_strsplit_set(folded, " \t\r\n", -1) : NULL;

  GArray *all = g_array_sized_new(FALSE, FALSE, sizeof(dt_capability_match_t),
                                  G_N_ELEMENTS(_capabilities));
  for(size_t k = 0; k < G_N_ELEMENTS(_capabilities); k++)
  {
    const dt_capability_descriptor_t *capability = &_capabilities[k];
    if(context && !(capability->contexts & context))
      continue;
    const int score = *folded ? _score_capability(capability, folded, tokens)
                              : (int)(G_N_ELEMENTS(_capabilities) - k);
    if(score > 0)
    {
      const dt_capability_match_t match = {capability, score};
      g_array_append_val(all, match);
    }
  }
  g_array_sort(all, _compare_matches);
  const size_t count = MIN((size_t)all->len, max_matches);
  if(count)
    memcpy(matches, all->data, count * sizeof(*matches));
  g_array_free(all, TRUE);
  return count;
}

const char *
dt_capability_side_effect_name(const dt_capability_side_effect_t value)
{
  static const char *const names[] = {"read_only", "reversible_edit",
                                      "library_mutation", "file_output",
                                      "destructive"};
  return value <= DT_CAPABILITY_DESTRUCTIVE ? names[value] : "unknown";
}

const char *dt_capability_exposure_name(const dt_capability_exposure_t value)
{
  static const char *const names[] = {"internal_only", "discovery", "read_only",
                                      "previewable", "executable"};
  return value <= DT_CAPABILITY_EXECUTABLE ? names[value] : "unknown";
}

const char *
dt_capability_argument_type_name(const dt_capability_argument_type_t value)
{
  static const char *const names[] = {"boolean", "integer", "number", "string",
                                      "enum"};
  return value <= DT_CAPABILITY_ARGUMENT_ENUM ? names[value] : "unknown";
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
