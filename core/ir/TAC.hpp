// https://en.wikipedia.org/wiki/Dominator_(graph_theory)
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/context/Type.hpp>
#include <core/parser/AstVisitor.hpp>


// SSA (Static Single Assignment) TAC (Three Adress Code)
namespace blast::core::ir::tac {

using BlockId = uint32_t;
using ValueId  = uint32_t;
using FctId  = uint32_t;

// Labels: prefix '%' for local and '@' for global

struct Operand {
    enum class Kind {
        NONE,
        BLOCK,
        REGISTER,
        LITERAL,
    };
    Kind m_kind;
    union {
        ValueId  m_value;   // REGISTER
        BlockId  m_block;   // BLOCK
        int64_t  m_imm;     // LITERAL
    };
};

inline Operand NONE() {
    return {
        .m_kind = Operand::Kind::NONE,
        .m_imm = 0
    };
}

inline Operand REGISTER(ValueId v) {
    return {
        .m_kind = Operand::Kind::REGISTER,
        .m_value = v
    };
}

inline Operand INT_LIT(std::int64_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_imm = v
    };
}


// Opcodes are grouped by role and kept contiguous within each group, as with
// ASTNode::Kind, so classifying an instruction stays a range check. The
// terminators come last and isTerminator() depends on that: keep new opcodes
// inside their group.
//
// Opcodes are not typed: there is one ADD, not ADD/FADD. Whether it is an
// integer or a floating-point add is decided by the type of its operands,
// which the type system already knows. The alternative (LLVM's) is to encode
// the type in the opcode; that duplicates information and doubles the enum.
enum class Opcode : std::uint8_t {
    // Arithmetic: result = lhs op rhs
    ADD,
    SUB,
    MUL,
    DIV,
    NEG,        // unary, reads lhs only

    // Comparison: result is a Bool
    // One opcode per relation rather than a single REL with a side field,
    // so a switch over the opcode is exhaustive on its own.
    LT,
    LE,
    GT,
    GE,
    EQ,
    NE,

    // Copy: result = lhs
    COPY,

    // Call
    // Multiple dispatch makes this central, and it needs N arguments, which
    CALL,

    // Terminators: exactly one, always last in a block
    BR,         // unconditional; lhs = target BLOCK
    CBR,        // conditional; lhs = condition, rhs = then BLOCK, result = else BLOCK
    RET,        // lhs = returned value, or NONE
};

constexpr bool isTerminator(Opcode op) {
    return op >= Opcode::BR;
}

constexpr bool isComparison(Opcode op) {
    return op >= Opcode::LT && op <= Opcode::NE;
}

struct Instruction {
    Operand m_result;
    Operand m_lhs;
    Operand m_rhs;
    Opcode  m_op;
};

struct BasicBlock {
    // Block inputs (entry block's are the fn args)
    std::vector<ValueId>     m_params;
    // Set of instructions in the block
    std::vector<Instruction> m_instrs;
    // Block parents to get Control flow Graph structure
    std::vector<BlockId>     m_preds;
};

struct Function {
    FctId m_id;
    // Assembly-compatible Mangled Name
    std::string             m_name;
    const context::FunctionType* m_type;
    std::vector<BasicBlock> m_blocks;   // block 0 is the entry
    // Every values in the function, their ValueId maps to their type (Int, Float, ...)
    // Values in a Function can be used my ALL blocks of the Function
    std::vector<const context::Type*> m_valueTypes;
};


// Translate AST to TAC
class ASTtoTAC: public parser::AstVisitor<ASTtoTAC, Operand> {
public:
    Function run(const parser::TranslationUnit& unit);

    Operand visitIntLiteral(const parser::IntLiteral& node);
    Operand visitIdentifier(const parser::Identifier& node);
    Operand visitBinaryExpr(const parser::BinaryExpr& node);
    //Operand visitAssign(const parser::Assign& node);
    //Operand visitVarDecl(const parser::VarDecl& node);
    Operand visitExprStmt(const parser::ExprStmt& node);
    Operand visitTranslationUnit(const parser::TranslationUnit& node);

public:

        

private:
    Function m_fn;
    // Name -> the value currently bound to it
    std::unordered_map<std::string, Operand> m_vars;
};

std::string dump(const Function& fn);

} // namespace blast::core::ir::tac