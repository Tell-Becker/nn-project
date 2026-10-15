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