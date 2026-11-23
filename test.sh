#!/bin/sh
# Run the C engine, search regression, and bounded-slice suites.
set -eu
cd "$(dirname "$0")"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
for test in test_regex test_literal_sets test_matchan; do
  # Deliberate word splitting lets CC/CFLAGS contain compiler arguments.
  ${CC:-cc} ${CFLAGS:--std=c11 -Wall -Wextra -pedantic -O2} "regex/$test.c" -o "$tmp/$test"
  "$tmp/$test"
done
