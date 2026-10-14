# AI Use Notes

The course asks students to record AI use, including prompts and the model
used. This file is a record of how Copilot has helped with this project so
far. The task descriptions below summarize the work; they are not exact
copies of every prompt.

## Tool

GitHub Copilot in VS Code. And ChatGPT to understand code.

## Help used

Copilot was used to help with:

- Understanding and changing the B0/B1 build and test setup.
- Writing and checking C tests and matrix-kernel code.
- Running tests, understanding test results, and explaining what the results
  do and do not show.
- Drafting and simplifying these project notes.
- Adding the host-only Xqmac8 reference and its directed and randomized tests.
- Adding the Xqmac8 instruction encoding, its Spike extension, and a RISC-V
  test that compares instruction results with the C reference.

## Prompt record

- Draft and refine the DeiT-Tiny design rationale and a staged implementation plan in `Design.md`.
- Explain the bare-metal build flow and the role of Make, the compiled ELF, and Spike.
- Change `src/test_mac/Makefile` to build RV64IMC+Zicsr/LP64 B0 (`-O0`) and B1 (`-O2`) variants with separate disassemblies.
- Implement the initial INT8 dot-product and small matrix-multiply correctness harness in `src/test_mac/test_mac.c`, including reference calculations, a signed-byte boundary case, and deterministic test data.
- Extract the matrix-multiply kernel into `src/test_mac/mac_kernels.c` and add a provisional DeiT-Tiny projection benchmark with B0/B1 targets. Host GCC builds, RV64 builds, Spike runs, and reference checks passed.
- Add a C reference for eight packed signed-byte products with a 64-bit wrapping accumulator, directed cases, 1,000 deterministic randomized cases, and a Make target. The task was to implement a small step after B1 without yet adding the Spike instruction.
- Implement the next Xqmac8 step: add the R-type custom-0 encoding, implement that operation in a Spike extension, and add a target test with directed and randomized comparisons against the software reference.
