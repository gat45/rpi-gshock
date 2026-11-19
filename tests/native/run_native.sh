#!/usr/bin/env bash
# Build and run the CasioLab host tests.
#
# Only the decoder/export layer is compiled here: it has no Flipper dependency,
# so the same code that ships in the FAP is exercised on the host.
set -euo pipefail

cd "$(dirname "$0")/../.."
OUT="dist/native"
mkdir -p "$OUT"

CC="${CC:-gcc}"
SRC=(
    protocol/drsp.c
    protocol/sample_capture.c
    data/lifelog_record.c
    data/watch_name_decoder.c
    data/battery_decoder.c
    data/time_decoder.c
    data/live_state.c
    data/data_manager.c
    storage/export.c
    tests/native/main.c
    tests/native/test_drsp.c
    tests/native/test_decoders.c
    tests/native/test_export.c
    tests/native/test_live.c
    tests/native/test_regress.c
)

"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror -I . -o "$OUT/casiolab_tests" "${SRC[@]}"
"$OUT/casiolab_tests"
