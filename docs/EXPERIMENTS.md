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

Next
Profile one real DeiT-Tiny layer (instruction mix, MACs per instruction) in B0/B1.
Put Xqmac8 into the matmul kernel and measure the instruction count against B1.
Add Xrequant8 once we know how much requantization costs.
Not done yet
Anything on a real model
Spike instruction-count numbers
RTL, cycle counts, area