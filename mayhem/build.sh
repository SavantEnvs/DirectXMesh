#!/usr/bin/env bash
# mayhem/build.sh — DirectXMesh: sanitized library + 8 libFuzzer harnesses (+ standalone
# reproducers), and a normal-flags library + known-answer selftest for mayhem/test.sh.
# Idempotent and offline: dependencies come from /opt/toolchains/deps (see mayhem/Dockerfile).
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${DXMESH_DEPS_PREFIX:=/opt/toolchains/deps/directxmath-6ac6a3c51d4d99237656f22486ae5177c14c565d;/opt/toolchains/deps/directx-headers-61c7718954ec73dbdd43c723240fe1a0452a3724}"
export CC CXX
cd "$SRC"

DXMATH="${DXMESH_DEPS_PREFIX%%;*}"
DXHDR="${DXMESH_DEPS_PREFIX##*;}"
OUT="$SRC/mayhem/out"
mkdir -p "$OUT"

# upstream CMake build of the library (Linux path: find_package(directxmath/directx-headers)).
# Precompiled headers off: they are not cache-safe and change nothing semantically.
cmake_lib() { # <build dir> <cxx flags> [extra cmake args]
  cmake -S "$SRC" -B "$1" -G Ninja -DCMAKE_BUILD_TYPE=None -DBUILD_TOOLS=OFF -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON -DCMAKE_PREFIX_PATH="$DXMESH_DEPS_PREFIX" \
    -DCMAKE_CXX_FLAGS="$2" "${@:3}"
  cmake --build "$1" -j"$MAYHEM_JOBS" --target DirectXMesh
}

# UBSan: two checks are turned off on five library sources only (mayhem/ubsan_scope.cmake names each
# source, the reproducing frame and the benign pattern): alignment in Clean/Validate/VBReader/VBWriter,
# nonnull-attribute in MeshletGenerator. ASan and the rest of UBSan stay on and halting everywhere.
# SanCov on the library and on the harness TUs (WaveFrontReader.h is header-only, so its parser
# code is compiled into the harness objects) — unconditionally, even with empty SANITIZER_FLAGS.
FUZZ_CFLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS"

# 1) sanitized + coverage-instrumented + DWARF-3 library for the fuzzers
cmake_lib "$SRC/mayhem/build-fuzz" "$FUZZ_CFLAGS" \
  -DCMAKE_PROJECT_DirectXMesh_INCLUDE="$SRC/mayhem/ubsan_scope.cmake"
LIB="$SRC/mayhem/build-fuzz/lib/libDirectXMesh.a"
[ -f "$LIB" ] || LIB="$(find "$SRC/mayhem/build-fuzz" -name 'libDirectXMesh.a' | head -1)"

INC=(-I"$SRC/DirectXMesh" -I"$SRC/Utilities" -I"$SRC/mayhem/harnesses"
     -I"$DXMATH/include" -I"$DXHDR/include" -I"$DXHDR/include/directx"
     -I"$DXHDR/include/wsl/stubs" -DUSING_DIRECTX_HEADERS -std=c++17 -Wno-macro-redefined)

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"

# 2) harnesses: libFuzzer binary + standalone reproducer each
for t in obj mtl vbo validate meshlets adjacency optimize vbreader; do
  $CXX $FUZZ_CFLAGS "${INC[@]}" -c "$SRC/mayhem/harnesses/fuzz_$t.cpp" -o "$OUT/fuzz_$t.o"
  $CXX $FUZZ_CFLAGS $LIB_FUZZING_ENGINE "$OUT/fuzz_$t.o" "$LIB" "$OUT/lsan_off.o" -o "$OUT/fuzz_$t"
  $CXX $FUZZ_CFLAGS "$OUT/standalone_main.o" "$OUT/fuzz_$t.o" "$LIB" "$OUT/lsan_off.o" -o "$OUT/fuzz_$t-standalone"
done

# 3) normal-flags library + known-answer selftest for mayhem/test.sh
cmake_lib "$SRC/mayhem/build-test" "-O1 $COVERAGE_FLAGS"
TLIB="$(find "$SRC/mayhem/build-test" -name 'libDirectXMesh.a' | head -1)"
$CXX -O1 $COVERAGE_FLAGS "${INC[@]}" "$SRC/mayhem/harnesses/selftest.cpp" "$TLIB" -o "$OUT/dxmesh_selftest"
echo "build.sh: done"
