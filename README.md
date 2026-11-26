# UTF-8 on x86-64: SSE2 with a scalar fallback

A strict, length-delimited UTF-8 validator in GNU assembly, with a small C API,
an independently written scalar reference, and a file-checking CLI. It accepts
ASCII NUL, rejects overlong encodings, UTF-16 surrogates, code points above
U+10FFFF, stray or malformed continuations, and truncated sequences. It checks
encoding only: it does not normalize text, forbid Unicode noncharacters, locate
an error, or retain state across stream chunks.

## Build and run

Linux x86-64, GNU-compatible assembler, C11 compiler, and GNU Make.
SSE2 is part of the x86-64 baseline; no runtime ISA detection is needed on
this target.

```sh
make
printf 'ASCII and \342\202\254\n' >/tmp/utf8-example-good
./utf8check /tmp/utf8-example-good
printf '\360\220\200' >/tmp/utf8-example-bad
./utf8check /tmp/utf8-example-bad
make test
make bench
```

Exit status: 0 valid, 1 invalid, 2 usage or I/O failure. The CLI uses
memory proportional to file size; the library itself uses constant extra space.

## Design

`utf8_sse2(p, n)` follows the SysV AMD64 calling convention and returns
1 or 0. It reads a 16-byte block with an unaligned SSE2 load only when
`n >= 16`. A zero `pmovmskb` mask means the entire block is ASCII.
Otherwise, the lowest set bit gives the offset of the first non-ASCII
byte, so validation can resume there without rechecking the ASCII prefix.
The scalar state checks length before reading continuation bytes, then checks
restricted second-byte ranges for `E0`, `ED`, `F0`, and `F4`. It stays scalar
through consecutive multibyte characters; after consuming the next ASCII
byte it checks whether another 16-byte block can be scanned. No padding or
alignment is required. A null pointer is only allowed at length zero.

The C reference reconstructs the code point and checks its minimum permitted
value for the encoded width, as well as Unicode scalar bounds. That makes
the oracle structurally different from the assembly implementation. The
two functions are not intended to be constant-time.

## Evidence

`make test` passed **4,515,229 checks**: every one- and two-byte input,
every legal Unicode scalar plus the surrogate range encoded as a negative
case, truncated multibyte encodings, deterministic random and mutated buffers,
explicit malformed inputs spanning alignment offsets, and protected guard
pages immediately before and after valid memory.

Measured on a Linux AMD EPYC 9V74 host, GCC 14.2.0, C baseline at
`-O3`. Samples measure validation only, not I/O. Each row uses the median
of seven batches of 48 calls over a reused 1 MiB synthetic buffer; execution
order alternates per batch. These are local microbenchmarks, not production
throughput or a comparison to other SIMD libraries.

| Input | C scalar (GiB/s) | SSE2 (GiB/s) | Ratio |
|---|---:|---:|---:|
| ASCII | 1.02 | 24.70 | 24.22× |
| 4-byte scalar every 64 bytes | 1.07 | 9.54 | 8.88× |
| Repeated 4-byte scalar | 1.40 | 1.83 | 1.31× |

The largest gain comes from skipping all-ASCII blocks. A first implementation
that returned to SSE2 after every code point achieved 0.99 GiB/s on the
all-4-byte workload, so this version avoids that redundant vector work. The
tradeoff is that it consumes one ASCII byte in the scalar loop when a
multibyte run ends. Real documents, cold caches, invalid inputs, and CPUs
with different branch/ISA costs can change the ranking.

Portfolio lab. No production deployment or independent security audit.
