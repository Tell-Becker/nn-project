Design:

Quantized DeiT-Tiny on a bare-metal RV64IMC core (CV-Wally), with a couple of small custom instructions to cut down the instruction count of the matrix multiplies.

The plan is: get a plain C version working, profile it in Spike, add instructions where the profile says they help, then implement the same instructions in the CV-Wally RTL and measure cycles and area.

1. Why DeiT-Tiny

It is a transformer (someting that was recommended in the course syllabus), so most of the work is plain matrix multiplication, which is exactly what a packed INT8 MAC instruction helps with. It is also in the size range the course asks for (about 5.7M parameters, a bit over 1 GMAC). A CNN would need im2col or direct convolution kernels on top of that. I would rather spend the time on the instructions. 

2. The workload

DeiT-Tiny at 224x224: 16x16 patches give 196 tokens plus one class token, so 197 tokens. 12 encoder layers, embedding size 192, 3 heads (head size 64), MLP hidden size 768.

Rough MAC count per operation:

Op	Shape (M x K x N)	MACs
Patch embed	196 x 768 x 192	29M (once)
QKV projection	197 x 192 x 576	21.8M per layer
Attention Q·Kᵀ	3 x (197 x 64 x 197)	7.5M per layer
Attention ·V	3 x (197 x 197 x 64)	7.5M per layer
Output projection	197 x 192 x 192	7.3M per layer
MLP fc1	197 x 192 x 768	29.1M per layer
MLP fc2	197 x 768 x 192	29.1M per layer

That is about 1.26 GMAC total. Roughly 85% is in the linear layers and 15% in the two attention matmuls. Softmax, GELU and LayerNorm are tiny in MACs, but on a core with no FPU they could still cost a surprising number of instructions. Checking that is one of the first profiling jobs.

Memory: the weights are around 5.7 MB as INT8. The largest activation is the MLP hidden layer, 197 x 768 = about 150 KB. All buffers will be static. Whether this fits depends on the CV-Wally memory setup.

3. Software plan

Quantization. INT8 weights and INT8 activations (W8A8), INT32 accumulators, symmetric weights, per-tensor activation scales to start with. Requantization uses the usual fixed-point multiplier plus shift, so no floating point at run time.

Non-linear ops without an FPU. Softmax, GELU and LayerNorm need integer versions. I plan to look at the integer-only approaches from I-ViT or similar and pick something simple. Accuracy only needs to be checked against my own reference, not retrained.

Kernel layout. Weights get packed offline so the inner loop reads 8 bytes at a time. All the K dimensions here (192, 768, 64) are multiples of 8. The attention ·V matmul has K = 197, so that one needs padding to 200.

Build variants (same as the course table):

Name	What it is
B0	Plain C, -O0
B1	Same C, -O2, plus hand optimizations (unrolling, loop order)
X1	B1 + Xqmac8
X2	X1 + Xrequant8
X3+	Whatever extension the profile points to

The order matters. Each one is added on top of the previous one so the gain from each instruction can be measured separately.

4. Custom instructions
Xqmac8

rd = rd + sum(a[i] * w[i]) for i in 0..7, where a and w are the eight signed bytes in rs1 and rs2. Lane 0 is the lowest byte. The sum wraps modulo 2^64.

Encoding: R-type in the custom-0 space (opcode 0x0b), funct3 = 0, funct7 = 0. It is used from C with .insn r (see custom_isa.h).

Things I have decided:

64-bit accumulator, even though the network only needs 32 bits. It keeps the semantics simple and the register is 64 bits anyway.
No saturation inside the instruction. The INT8 products summed over 768 elements cannot get near overflow in 32 bits.

5. Evaluation

ISA level (Spike): total instructions, instruction mix, MACs per instruction, loads/stores per MAC, code size, how often the custom instructions are used. Done for B0, B1, X1, X2.

Hardware level (CV-Wally RTL): cycle count and CPI for at least one kernel (probably a 197 x 192 x 192 projection or a smaller slice of it), Fmax or critical path, and area from Synopsys DC with SAED 32/28nm. Final number: speedup divided by area.

Spike counts instructions, not cycles, so ISA-level gains and RTL cycle gains will be reported separately.

Target from the assignment: any one of 2x fewer instructions on a GEMM kernel, 2x kernel cycle speedup, or 1.5x end-to-end. For the 8-MAC instruction, the inner loop is about ld, ld, xqmac8, addi, addi, branch per 8 MACs, so I expect the 2x kernel target to be reachable and the end-to-end one to depend on how much non-GEMM work is left.

6. Extension candidates

Chosen after profiling, not before. Current guesses:

Post-increment load. In the loop above, half the instructions are pointer increments and the branch. This should be the obvious next thing after Xqmac8.
Hardware loops. Removes the branch and counter.
W8A16. Only if there is time, as an accuracy/performance comparison.

Cache-side extensions are a maybe, depending on whether the RTL runs show memory stalls.

7. Milestones
Stage	What	Status
B0/B1 harness, dot and matmul tests	src/test_mac	done
Xqmac8 C reference	host tests	done
Xqmac8 in Spike	extension + test	done
Xqmac8 in a tiny matrix kernel	3 x 8 by 8 x 4, compare with scalar B0/B1	done
Profile a real DeiT-Tiny layer in Spike		[todo]
Full model in C (B0/B1)		[todo]
Xqmac8 in the kernels (X1)		[todo]
Xrequant8 (X2)		[todo]
Midterm report		[date]
Xqmac8 in CV-Wally RTL + kernel run		[todo]
Synthesis		[todo]
Extension		[todo]
Final report		[todo]

8. Risks
Integer softmax/GELU/LayerNorm might eat the speedup from the matmuls (Amdahl).
The three-read-port issue could make the RTL change bigger than expected.
Full model may not fit in whatever memory the CV-Wally testbench provides. If so, fall back to one transformer block, which the assignment allows