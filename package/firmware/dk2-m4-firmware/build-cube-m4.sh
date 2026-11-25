#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu

ide=$1
build_dir=$2
source_dir=$build_dir/source
import_dir=$3
build_selector=$4
elf_path=$5

fail() {
    echo "dk2-m4-firmware: $*" >&2
    exit 1
}

[ -n "$build_selector" ] && [ -n "$elf_path" ] ||
    fail "set M4_BUILD to ProjectName/Configuration and M4_ELF to the source-relative ELF path"
command -v "$ide" >/dev/null ||
    fail "set STM32CUBEIDE to the installed stm32cubeide executable"

import_path=$(realpath -m "$source_dir/$import_dir")
elf=$(realpath -m "$source_dir/$elf_path")
case "$import_path" in
    "$source_dir"|"$source_dir"/*) ;;
    *) fail "M4_IMPORT must stay inside firmware/cube-m4" ;;
esac
case "$elf" in
    "$source_dir"/*.elf) ;;
    *) fail "M4_ELF must name an .elf inside firmware/cube-m4" ;;
esac
[ -d "$import_path" ] || fail "M4_IMPORT directory does not exist: $import_dir"

workspace=$(mktemp -d "$build_dir/workspace.XXXXXX")
trap 'rm -rf -- "$workspace"' EXIT
trap 'exit 1' HUP INT TERM

# Require the IDE to produce a fresh ELF in the staged source tree.
rm -f -- "$elf"
unset MAKEFLAGS MFLAGS
"$ide" --launcher.suppressErrors -nosplash     -application org.eclipse.cdt.managedbuilder.core.headlessbuild     -data "$workspace" -no-indexer -printErrorMarkers     -importAll "$import_path" -cleanBuild "$build_selector"

[ -s "$elf" ] || fail "CubeIDE did not produce M4_ELF: $elf_path"
LC_ALL=C readelf -h -A "$elf" > "$build_dir/elf-info"
grep -Eq 'Type:[[:space:]]+EXEC' "$build_dir/elf-info" &&
    grep -Eq 'Tag_CPU_arch:[[:space:]]+v7E-M$' "$build_dir/elf-info" ||
    fail "expected an executable Armv7E-M ELF"
install -m 0644 "$elf" "$build_dir/dk2-m4.elf"
