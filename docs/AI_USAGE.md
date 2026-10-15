AI Usage:
The course asks for a record of AI use. These are paraphrased summaries of what I asked for, not exact prompts.

Tools
GitHub Copilot in VS Code
ChatGPT, for understanding code
Claude, for understanding and verifying if design documents fit what course wants
What AI helped with:
Build and test setup for B0/B1 (Makefile, flags, disassembly targets)
Writing and checking C tests and the matmul kernel code
Running tests and explaining results
The Xqmac8 C reference, the instruction encoding, the Spike extension, and the Spike test
The small scalar and Xqmac8 matrix-kernel test and its Make target
Asked how the bare-metal flow works: Make, the ELF, and Spike.
Changed src/test_mac/Makefile to build B0 (-O0) and B1 (-O2) for RV64IMC + Zicsr with separate disassemblies.
Wrote the first INT8 dot product and small matmul tests in test_mac.c, with reference versions and a boundary case.
Moved the matmul into mac_kernels.c and added a provisional DeiT-Tiny projection benchmark. Host build, RV64 build, and Spike all passed.
Added the host-only Xqmac8 reference with directed and random tests.
Added the Xqmac8 encoding, the Spike extension, and a test comparing it with the C reference.