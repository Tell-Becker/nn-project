Experiments:
Running log of what has been run and what the results mean. Newest at the bottom.

B0 / B1 smoke test (Oct 6)

Small test program in src/test_mac, built for rv64imc_zicsr / lp64. B0 is -O0, B1 is -O2. Both run in Spike and print the same thing:

text
MAC result: -4
Dot product tests passed: boundary + 64 deterministic cases
Matrix multiply passed: 3 x 5 by 5 x 4

Only a sanity check of the dot product and a tiny matmul. Nothing here is DeiT-Tiny yet.

Xqmac8 C reference (Oct 12)

Plain C model of the instruction, run on the host. Eight signed bytes per operand, lane 0 in the low byte, sum added to a 64-bit accumulator that wraps.

sh
make -C src/test_mac xqmac8-host-test

Tests: 4 directed cases (all ones, mixed signs, -128 x -128 on every lane, accumulator wraparound) and 1000 random ones against a separate calculation. All passed.

This only tests the reference and the packing order.

Xqmac8 in Spike (Oct 15)

Spike extension for the custom-0 instruction, plus a RISC-V test that calls it through .insn and compares with scalar C.

sh
bash scripts/build_spike_xqmac8.sh
make -C src/test_mac xqmac8-spike-test

Same 4 directed cases, plus 512 random ones. All passed.

This shows the instruction does what the reference says in Spike. It says nothing about CV-Wally or cycles.

Xqmac8 in a tiny matrix kernel (Oct 15)

Added a 3 x 8 by 8 x 4 signed INT8 matrix-multiply test. The B0 and B1 builds
use ordinary C multiply-accumulate. The X1 build packs each row and column
into eight byte lanes and uses one Xqmac8 instruction per output. All three
builds use the same deterministic inputs and check every output against an
int64 reference.

sh
make -C src/test_mac xqmac8-matrix-test

This is a correctness integration test. The software packs values in the
kernel and the matrix is intentionally small, so it is not a performance
comparison or a representative model workload. Instruction profiling of a
larger kernel is still to be done.

Run on October 15, 2026: B0, B1, and X1 all passed with checksum `32584`.
The matching checksums are a convenient summary; each output was also checked
against the int64 reference.

Projection-shaped kernel profile (Oct 16)

The next step is a larger synthetic matrix workload shaped like a DeiT-Tiny
projection: 197 rows (tokens), 192 input features, and 192 output features.
That is 7,262,208 multiply-accumulate operations. These are deterministic
generated values, not weights or activations from a selected checkpoint.

The B0 and B1 scalar kernels and the X1 Xqmac8 kernel use the same pre-transposed
weight data. X1 reads groups of eight activation and weight bytes and executes
one Xqmac8 per group. Data generation, weight transposition, and reference
checking are outside the measured kernel. The test checks every output against
an int64 reference.

Run correctness tests with:

```sh
make -C src/test_mac projection-test
```

Collect function-local Spike instruction counts with:

```sh
make -C src/test_mac profile-projection
```

The profiler uses Spike's `-g` PC histogram and the function bounds and
disassembly from `nm` and `objdump`. Results are saved as JSON under `results/`.
This counts instructions, not cycles. The benchmark is only a shape-based
kernel experiment; it does not establish model accuracy or CV-Wally performance.

All three benchmark programs passed and produced the checksum `-10185794`.
The recorded function-local counts were:

| Build | Dynamic instructions | Xqmac8 instructions | Function bytes | MACs / instruction |
|---|---:|---:|---:|---:|
| B0 (`-O0`) | 298,887,825 | 0 | 268 | 0.02430 |
| B1 (`-O2`) | 51,101,613 | 0 | 106 | 0.14211 |
| X1 (`-O2`, Xqmac8) | 5,712,813 | 907,776 | 100 | 1.27121 |

For this kernel function, X1 used about 8.95 times fewer dynamic instructions
than B1 (about 88.8% fewer). This is not an 8.94x cycle speedup. The count is
limited to the matrix function; setup, transposition, reference calculation,
and output checking are excluded. The transposed weights are prepared before
the function, as if that layout conversion were done offline.

The tested shape is a provisional standard DeiT-Tiny projection shape only.
No checkpoint was selected, and the values are generated test data. The next
model-specific step is to confirm the exact tensor shape and data layout from
the selected checkpoint before treating this profile as a real model layer.