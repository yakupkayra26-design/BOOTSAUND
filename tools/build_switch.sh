#!/bin/sh
set -eu

docker run --rm \
  -v "$(pwd):/work" \
  -w /work \
  devkitpro/devkita64:latest \
  bash -lc 'dkp-pacman -S --needed --noconfirm switch-dev >/dev/null && make "$@"' \
  -- "$@"