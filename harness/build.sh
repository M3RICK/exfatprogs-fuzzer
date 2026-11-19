#!/bin/sh
# Build every fuzz_*.c harness with ASan + UBSan + libFuzzer, linked against
# the exfatprogs library sources.
#
# UBSan catches what ASan cannot see, such as out-of-range shifts on the
# boot-sector geometry. -fno-sanitize-recover turns undefined behaviour into
# an abort, so libFuzzer saves the reproducer instead of logging and moving on.
set -e
T=../targets/exfatprogs
mkdir -p bin

for src in fuzz_*.c; do
	out="bin/${src%.c}"
	# Targeted UBSan checks only: geometry arithmetic bugs (shifts, size and
	# offset overflows, division by zero, array bounds). The full `undefined`
	# group is noisy here (offsetof idiom, packed on-disk structs).
	clang -g -O1 -fsanitize=address,fuzzer \
		-fsanitize=shift,signed-integer-overflow,integer-divide-by-zero,bounds \
		-fno-sanitize-recover=all \
		-I"$T/include" -I"$T" \
		"$src" image.c "$T"/lib/*.c \
		-o "$out"
	echo "OK -> ./$out"
done
