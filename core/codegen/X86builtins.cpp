#include <core/codegen/X86.hpp>
#include <core/Exception.hpp>
#include <limits>

namespace blast::core::codegen {

std::string blast_print_name(const ir::Type& type) {
    return std::string("blast_print_") + (type.m_kind == ir::Type::Kind::UINT ? "u" : "i") + std::to_string(ir::bits(type.m_width));
}

// Print the content of 'RDI' as an INT
MachineFunction blast_print_int(const X86& x86, const ir::Type& type) {
    // To print a i64, we can store the format string ("%ld\n") on the stack
    // X86 store lowest bytes first:
    // 5 6C 64 0A (%, l, d, newline) read back as the number 0x0A646C25
    std::size_t format_code = 0x00000025; // %
    switch (type.m_kind) {
        case ir::Type::Kind::INT: {
            switch (type.m_width) {
                case ir::Type::Width::W1:
                    format_code |= 0x00006300; // 0x63 = 'c'
                    break;
                case ir::Type::Width::W8:
                case ir::Type::Width::W16:
                case ir::Type::Width::W32:
                    format_code |= 0x00006400; // 0x64 = 'd'
                    break;
                case ir::Type::Width::W64:
                case ir::Type::Width::W80:
                case ir::Type::Width::W128:
                case ir::Type::Width::W256:
                case ir::Type::Width::W512:
                    format_code |= 0x00646C00; // 0x64 = 'd', 0x6C = 'l'
                    break;
            }
            break;
        }

        case ir::Type::Kind::UINT: {
            switch (type.m_width) {
                case ir::Type::Width::W1:
                    format_code |= 0x00006300; // 0x63 = 'c'
                    break;
                case ir::Type::Width::W8:
                case ir::Type::Width::W16:
                case ir::Type::Width::W32:
                    format_code |= 0x00007500; // 0x75 = 'u'
                    break;
                case ir::Type::Width::W64:
                case ir::Type::Width::W80:
                case ir::Type::Width::W128:
                case ir::Type::Width::W256:
                case ir::Type::Width::W512:
                    format_code |= 0x00756C00; // 0x75 = 'u', 0x6C = 'l'
                    break;
            }
            break;
        }

        // Default: i64
        default:
            format_code = 0x00646C25;
            break;
    }

    // TODO: REMOVE
    // Add new line
    format_code |= 0x0A000000;

    std::string fct_name = blast_print_name(type);

    // Create the function
    MachineFunction fct(
        // id makes no sense here
        std::numeric_limits<ir::FctId>::max(),
        fct_name
    );
    // Create its entry block
    MachineBlock& entry = fct.addBlock(0, fct_name + "__entry");
    // To print a i64, we can store the format string ("%ld\n") on the stack
    // X86 store lowest bytes first:
    // 5 6C 64 0A (%, l, d, newline) read back as the number 0x0A646C25

    // SUB rsp, 16 (make room for the format string, needs only 5 bytes but we pad it to 16)
    entry.addInstruction(MachineOpcode::SUB, x86.getPREG("RSP"), LIT({ .m_i64 = 16 }, ir::PTR()));
    // MOV rax, <str format code>
    const MachineOperand rax = x86.fixedVreg(fct, "RAX");
    entry.addInstruction(MachineOpcode::MOV, rax, LIT({ .m_i64 = format_code }, ir::INT(ir::Type::Width::W64)));
    // MOV [rsp], rax
    entry.addInstruction(MachineOpcode::MOV, MEM(&x86.getReg("RSP"), 0, ir::INT(ir::Type::Width::W64)), rax);
    // MOV rsi, rdi
    entry.addInstruction(MachineOpcode::MOV, x86.fixedVreg(fct, "RSI"), x86.fixedVreg(fct, "RDI"));
    // MOV rdi, rsp
    entry.addInstruction(MachineOpcode::MOV, x86.fixedVreg(fct, "RDI"), x86.getPREG("RSP"));
    // XOR eax, eax
    const MachineOperand eax = x86.fixedVreg(fct, "EAX");
    entry.addInstruction(MachineOpcode::XOR, eax, eax);
    // CALL printf
    entry.addInstruction(MachineOpcode::CALL, SYM("printf", ir::PTR()), MNONE());
    // RET
    entry.addInstruction(MachineOpcode::RET, MNONE(), MNONE());

    return fct;
}

} // namespace blast::core::codegen
