#!/usr/bin/env sh
set -eu

if ! command -v emcc >/dev/null 2>&1; then
  echo "error: emcc is required. Install and activate Emscripten, then rerun js/build.sh." >&2
  exit 1
fi

cd "$(dirname "$0")"
mkdir -p build

if [ -z "${EMSDK_PYTHON:-}" ]; then
  for py in /opt/homebrew/bin/python3 /opt/homebrew/opt/python@3.14/bin/python3.14 /opt/homebrew/opt/python@3.12/bin/python3.12 python3; do
    if command -v "$py" >/dev/null 2>&1; then
      EMSDK_PYTHON=$(command -v "$py")
      export EMSDK_PYTHON
      break
    fi
  done
fi

emcc regex_wasm.c -O3 \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sENVIRONMENT=web \
  -sFILESYSTEM=0 \
  -sEXPORTED_FUNCTIONS='["_match","_matcha","_label","_regex_count","_regex_set_jump","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall","getValue","HEAPU8","HEAP32"]' \
  -o build/regex_wasm.js
