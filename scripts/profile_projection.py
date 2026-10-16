#!/usr/bin/env python3
"""Count dynamic Spike instructions inside one named function."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess


HISTOGRAM_LINE = re.compile(r"^\s*([0-9a-fA-F]+)\s+(\d+)\s*$")


def run(command, *, env=None, combined=False):
    result = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    return result.stdout + "\n" + result.stderr if combined else result.stdout


def get_symbol(elf, symbol, nm):
    for line in run([nm, "-S", "--defined-only", str(elf)]).splitlines():
        fields = line.split()
        if len(fields) >= 4 and fields[-1] == symbol:
            return int(fields[0], 16), int(fields[1], 16)
    raise ValueError(f"Function {symbol!r} not found in {elf}")


def get_disassembly(elf, objdump):
    instructions = {}
    for line in run([objdump, "-d", str(elf)]).splitlines():
        if ":" not in line:
            continue
        address_text, encoded_text = line.split(":", 1)
        try:
            address = int(address_text.strip(), 16)
        except ValueError:
            continue
        fields = encoded_text.split()
        if len(fields) < 2 or not re.fullmatch(r"[0-9a-fA-F]{4,8}", fields[0]):
            continue
        encoding = int(fields[0], 16)
        mnemonic = fields[1].lower()
        if len(fields[0]) == 8 and (encoding & 0x7f) == 0x0b:
            funct3 = (encoding >> 12) & 0x7
            funct7 = (encoding >> 25) & 0x7f
            if funct3 == 0 and funct7 == 0:
                mnemonic = "xqmac8"
        instructions[address] = mnemonic
    return instructions


def classify(mnemonic):
    if mnemonic in {"lb", "lbu", "lh", "lhu", "lw", "lwu", "ld",
                    "c.lb", "c.lbu", "c.lh", "c.lhu", "c.lw", "c.lwsp",
                    "c.ld", "c.ldsp"}:
        return "loads"
    if mnemonic in {"sb", "sh", "sw", "sd", "c.sb", "c.sh", "c.sw",
                    "c.swsp", "c.sd", "c.sdsp"}:
        return "stores"
    if mnemonic == "xqmac8":
        return "custom"
    if mnemonic.startswith(("mul", "c.mul")):
        return "multiply"
    if mnemonic.startswith(("b", "j", "c.j", "c.beq", "c.bne", "ret",
                            "jr", "jal")):
        return "branch"
    if mnemonic in {"ecall", "ebreak", "fence", "fence.i"}:
        return "other"
    return "integer_alu"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--spike", required=True)
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--function", required=True)
    parser.add_argument("--configuration", required=True)
    parser.add_argument("--compiler-flags", required=True)
    parser.add_argument("--work-units", required=True, type=int)
    parser.add_argument("--isa", default="rv64imc_zicsr")
    parser.add_argument("--extension")
    parser.add_argument("--libdir")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    start, size = get_symbol(args.elf, args.function, args.nm)
    end = start + size
    instructions = get_disassembly(args.elf, args.objdump)
    command = [args.spike, "-g", f"--isa={args.isa}"]
    if args.extension:
        command.append(f"--extension={args.extension}")
    command.append(str(args.elf))
    environment = os.environ.copy()
    if args.libdir:
        current = environment.get("LD_LIBRARY_PATH", "")
        environment["LD_LIBRARY_PATH"] = (
            f"{args.libdir}:{current}" if current else args.libdir
        )

    histogram_text = run(command, env=environment, combined=True)
    histogram = {}
    for line in histogram_text.splitlines():
        match = HISTOGRAM_LINE.match(line)
        if match:
            histogram[int(match.group(1), 16)] = int(match.group(2))
    if not histogram:
        raise ValueError("Spike emitted no PC histogram; check that -g is supported")

    counts = {name: 0 for name in
              ("loads", "stores", "integer_alu", "multiply", "branch",
               "custom", "other")}
    for address, count in histogram.items():
        if start <= address < end:
            if address not in instructions:
                raise ValueError(f"No disassembly at profiled PC 0x{address:x}")
            counts[classify(instructions[address])] += count

    total = sum(counts.values())
    if total == 0:
        raise ValueError(f"No instructions profiled for {args.function}")
    result = {
        "configuration": args.configuration,
        "elf": str(args.elf),
        "function": args.function,
        "function_size_bytes": size,
        "work_units": args.work_units,
        "dynamic_instructions": total,
        "custom_instructions": counts["custom"],
        "work_units_per_instruction": args.work_units / total,
        "instruction_mix": counts,
        "compiler_flags": args.compiler_flags,
        "command": command,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
