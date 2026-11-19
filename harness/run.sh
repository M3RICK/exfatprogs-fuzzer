#!/bin/sh
# Run one harness with the settings that matter. Extra arguments go to
# libFuzzer after the defaults, so they override them (last flag wins).
#
#   sh run.sh fuzz_exfat_dir
#   sh run.sh fuzz_exfat_tree -jobs=4 -max_total_time=3600
#   CORPUS=../corpus_tree sh run.sh fuzz_exfat_tree
set -e
cd "$(dirname "$0")"
target=${1:?usage: sh run.sh <fuzz_target> [libFuzzer flags...]}
shift
mkdir -p ../findings
# -max_len: the default 1 MiB cuts the 4 MiB seed and breaks its geometry.
exec ./bin/"$target" -max_len=4194304 -dict=../corpus.dict \
	-artifact_prefix=../findings/ "$@" "${CORPUS:-../corpus}"
