# regex web support

This directory makes `../regex/regex.c` usable in modern browsers through Emscripten and WebAssembly.

The browser wrapper exposes `match`, `matcha`, `label`, and `compile`, with `match` and `matcha` as the primary calls.

Offsets, labels, and compiled tokens are UTF-8 byte based to match the C library.

The demo visualizes the compiled token stream, graph structure, match highlighting, clicked-match alignment, and saved regex shortcuts.

Run `./build.sh`, then serve this directory from localhost before opening `demo.html`.
