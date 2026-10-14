#include <cstdint>

#include "custom_isa_ref.h"
#include "decode_macros.h"
#include "extension.h"
#include "insn_macros.h"

namespace {

constexpr insn_bits_t xqmac8_match = 0x0b;
constexpr insn_bits_t xqmac8_mask = 0xfe00707f;

static reg_t execute_xqmac8(processor_t *processor, insn_t insn, reg_t pc)
{
    require(processor->get_xlen() == 64);
    state_t *state = processor->get_state();
    reg_t accumulator = state->XPR[insn.rd()];
    reg_t packed_left = state->XPR[insn.rs1()];
    reg_t packed_right = state->XPR[insn.rs2()];

    state->XPR.write(insn.rd(),
                     xqmac8_reference(accumulator, packed_left, packed_right));
    return pc + 4;
}

struct rd_arg_t : public arg_t {
    std::string to_string(insn_t instruction) const override
    {
        return xpr_name[instruction.rd()];
    }
} rd_arg;

struct rs1_arg_t : public arg_t {
    std::string to_string(insn_t instruction) const override
    {
        return xpr_name[instruction.rs1()];
    }
} rs1_arg;

struct rs2_arg_t : public arg_t {
    std::string to_string(insn_t instruction) const override
    {
        return xpr_name[instruction.rs2()];
    }
} rs2_arg;

class xqmac8_extension_t : public extension_t {
public:
    const char *name() const override { return "xqmac"; }

    std::vector<insn_desc_t>
    get_instructions(const processor_t &) override
    {
        return {{xqmac8_match, xqmac8_mask,
                 execute_xqmac8, execute_xqmac8, execute_xqmac8,
                 execute_xqmac8, execute_xqmac8, execute_xqmac8,
                 execute_xqmac8, execute_xqmac8}};
    }

    std::vector<disasm_insn_t *>
    get_disasms(const processor_t *) override
    {
        return {new disasm_insn_t("xqmac8", xqmac8_match, xqmac8_mask,
                                  {&rd_arg, &rs1_arg, &rs2_arg})};
    }
};

}

REGISTER_EXTENSION(xqmac, []() {
    static xqmac8_extension_t extension;
    return &extension;
})
