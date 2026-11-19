# exfatprogs-fuzzer

libFuzzer harnesses for [exfatprogs](https://github.com/exfatprogs/exfatprogs),
the Linux exFAT filesystem utilities. Every byte of an exFAT image is attacker
input: reading an untrusted volume, such as a USB stick, should never crash or
misbehave on the host. There is prior art on this code, e.g. CVE-2023-45897,
an out-of-bounds access in the dentry-set parsing.

## Result

One bug found and fixed upstream.

`read_boot_sect()` used two `u8` fields of the boot sector, `sect_size_bits`
and `sect_per_clus_bits`, as shift counts before checking their range. A
crafted boot sector triggers undefined behaviour:

```
lib/libexfat.c:2035:16: runtime error: shift exponent 123 is too large for 32-bit type 'int'
```

`fsck` validated these fields earlier in its own path, but the other tools
(`dump`, `label`, `tune`, ...) reached `read_boot_sect()` with unchecked input.
During review the maintainer pointed out the same pattern in
`exfat_get_root_entry_offset()`; `exfat_set_volume_serial()` had it too. The fix
validates the fields in all three places (exFAT allows 512 B to 4 KiB sectors
and clusters up to 32 MiB).

- Pull request: [#403](https://github.com/exfatprogs/exfatprogs/pull/403)
- Commit: [f976d4d](https://github.com/exfatprogs/exfatprogs/commit/f976d4d0b033e72518fb67f1d805fd49f2aebd95)
- Released in [exfatprogs 1.4.5](https://github.com/exfatprogs/exfatprogs/releases/tag/1.4.5)

## Harnesses

All targets share `image.c`: the input bytes go into a `memfd`, which the
library opens by path like a real device, then the boot sector is parsed
through the public API (`exfat_get_blk_dev_info`, `exfat_alloc_exfat`).

| Target | What it exercises |
|--------|-------------------|
| `fuzz_exfat_boot` | Boot sector parsing and geometry arithmetic only |
| `fuzz_exfat_dir` | A lookup in the root directory, which parses every full dentry set (file `0x85`, stream `0xC0`, name `0xC1`) |
| `fuzz_exfat_tree` | Recursive walk into subdirectories through their start clusters, with depth and entry caps |

Each input gets a fresh context, freed at the end, so a crash is never caused
by leftover state.

## Notes

The first harness only iterated raw dentries and its coverage flattened fast.
Driving it through `exfat_lookup_file` instead made the library interpret full
dentry sets and broke that plateau. Both directory targets then saturated
(193 and 147 edges) without a crash.

The reason was the file entry checksum. Each file dentry set carries a checksum
that the library verifies before parsing the set further, so almost every
random mutation was rejected at that point. Before teaching the fuzzer about
the checksum, I first tried a surface that sits before any checksum: the boot
sector. That is `fuzz_exfat_boot`. ASan alone saw nothing there, because
the bug is an out-of-range shift, not a memory error. Adding UBSan's `shift`
check with `-fno-sanitize-recover` turned it into an abort that libFuzzer could
save.

Settings that mattered:

- `print_level = 0` mutes the library's own logging, which keeps throughput up
  and leaves stderr to the sanitizers.
- `-max_len` must be large enough not to truncate the seed below a valid exFAT
  geometry, otherwise coverage is silently capped.

## Build and run

Needs clang with ASan, UBSan and libFuzzer.

```sh
git clone --depth 1 https://github.com/exfatprogs/exfatprogs.git targets/exfatprogs
cd harness && sh build.sh
```

The seed is a small valid exFAT image with a few files in it (`mkfs.exfat`,
4 MiB minimum).

```sh
./bin/fuzz_exfat_dir -max_len=4194304 -dict=../corpus.dict -artifact_prefix=../findings/ ../corpus
```

## Disclosure

Every crash is reproduced, root-caused and reported to the maintainers before
anything is published here. Crashing inputs are never committed.
