#!/bin/sh

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
darktable_bin="$project_dir/build/bin/darktable"
profile_dir=${DARKTABLE_ESSENTIALS_PROFILE:-"$project_dir/build/essentials-profile"}

# Homebrew's GTK runtime data is not discovered automatically when a build-tree
# executable opens a native dialog outside an application bundle.
if command -v brew >/dev/null 2>&1; then
  brew_prefix=$(brew --prefix)
  brew_share="$brew_prefix/share"
  schema_dir="$brew_share/glib-2.0/schemas"
  if [ -f "$schema_dir/gschemas.compiled" ]; then
    GSETTINGS_SCHEMA_DIR="$schema_dir"
    export GSETTINGS_SCHEMA_DIR
  fi
  runtime_data_dirs="$brew_share"
  app_share="/Applications/darktable.app/Contents/Resources/share"
  if [ -d "$app_share/icons/Adwaita" ]; then
    runtime_data_dirs="$runtime_data_dirs:$app_share"
  fi
  XDG_DATA_DIRS="$runtime_data_dirs${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
  export XDG_DATA_DIRS
fi

if [ ! -f "$project_dir/build/CMakeCache.txt" ]; then
  echo "configure darktable first with: cmake -S . -B build" >&2
  exit 1
fi

cmake --build "$project_dir/build" --target darktable modulegroups masks exposure \
  colorbalancergb contrastntexture sharpen denoiseprofile \
  essentials_header essentials_library essentials_inspector -j4

mkdir -p "$profile_dir/config" "$profile_dir/cache" "$profile_dir/tmp"

# The development executable reads runtime assets from build/share. Keep the
# Essentials styling in sync without installing over the user's system copy.
mkdir -p "$project_dir/build/share/darktable/themes"
cp "$project_dir/data/themes/darktable.css" \
  "$project_dir/build/share/darktable/themes/darktable.css"

exec "$darktable_bin" \
  --datadir "$project_dir/build/share/darktable" \
  --moduledir "$project_dir/build/lib/darktable" \
  --configdir "$profile_dir/config" \
  --cachedir "$profile_dir/cache" \
  --tmpdir "$profile_dir/tmp" \
  --library "$profile_dir/library.db" \
  --conf ui/experience_mode=essentials \
  "$@"
