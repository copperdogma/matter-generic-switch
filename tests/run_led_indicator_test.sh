#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

${CC:-cc} -std=c11 -Wall -Wextra -Werror \
  -I"$repo_root/tests/fakes" \
  -I"$repo_root/firmware/main" \
  "$repo_root/firmware/main/drivers/led_indicator.c" \
  "$repo_root/tests/test_led_indicator.c" \
  -o "$tmp_dir/test_led_indicator"

"$tmp_dir/test_led_indicator"
