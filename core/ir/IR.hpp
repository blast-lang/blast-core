// https://en.wikipedia.org/wiki/Dominator_(graph_theory)
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/context/ASTContext.hpp>
#include <core/parser/AstVisitor.hpp>


// SSA (Static Single Assignment) TAC (Three Adress Code)
namespace blast::core::ir {


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
        int64_t  m_i64;     // LITERAL
    };
};

inline Operand NONE() {
    return {
        .m_kind = Operand::Kind::NONE,
        .m_i64 = 0
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
        .m_i64 = v
    };
}


// The single list every opcode-keyed table is generated from, so a new opcode
// cannot be added to one and forgotten in another. Order is load-bearing:
// isComparison() and isTerminator() below are range checks over it.
#define BLAST_IR_OPCODES(X)                     \
    /* Arithmetic: result = lhs op rhs */       \
    X(ADD) X(SUB) X(MUL) X(DIV)                 \
    X(NEG)  /* unary '-' */                     \
    /* Comparisons: LT .. NE */                 \
    X(LT) X(LE) X(GT) X(GE) X(EQ) X(NE)         \
    X(COPY) X(CALL)                             \
    /* Terminators: BR .. end */                \
    X(BR) X(CBR) X(RET)

enum class Opcode: std::uint8_t {
#define BLAST_IR_OPCODE_ENUMERATOR(name) name,
    BLAST_IR_OPCODES(BLAST_IR_OPCODE_ENUMERATOR)
#undef BLAST_IR_OPCODE_ENUMERATOR
};

constexpr bool isTerminator(Opcode op) {
    return op >= Opcode::BR;
}

constexpr bool isComparison(Opcode op) {
    return op >= Opcode::LT && op <= Opcode::NE;
}

class Instruction {
private:
    Operand m_result;
    Operand m_lhs;
    Operand m_rhs;
    Opcode  m_op;
    // Dumping debug
    std::string m_comment;

public:
    Instruction(Operand result, Operand lhs, Operand rhs, Opcode op):
        m_result(result), m_lhs(lhs), m_rhs(rhs), m_op(op), m_comment("")
    {}

    Operand result() const { return this->m_result; }
    Operand lhs() const { return this->m_lhs; }
    Operand rhs() const { return this->m_rhs; }
    Opcode  op() const { return this->m_op; }

    const std::string& comment() const { return this->m_comment; }
    void setComment(std::string comment) { this->m_comment = std::move(comment); }
};

class BasicBlock {
private:
    BlockId m_id;
    // Block inputs (entry block's are the fn args)
    std::vector<ValueId>     m_params;
    // Set of instructions in the block
    std::vector<Instruction> m_instrs;
    // Block parents to get Control flow Graph structure
    std::vector<BlockId>     m_preds;

public:
    BasicBlock(BlockId id): m_id(id), m_params(), m_instrs(), m_preds() {}
    BlockId id() const { return this->m_id; }

    std::vector<ValueId>& params() { return this->m_params; }
    const std::vector<ValueId>& params() const { return this->m_params; }

    std::vector<Instruction>& instrs() { return this->m_instrs; }
    const std::vector<Instruction>& instrs() const { return this->m_instrs; }

    std::vector<BlockId>& preds() { return this->m_preds; }
    const std::vector<BlockId>& preds() const { return this->m_preds; }

    Operand newInstruction(Operand lhs, Operand rhs, Opcode op) {
        // Generate a new valueId (register) for the current function
        ValueId new_vid = this->m_instrs.size();
        // And create the instruction for it
        // And store it as part of current block's instructions
        this->m_instrs.push_back(Instruction(REGISTER(new_vid), lhs, rhs, op));
        return REGISTER(new_vid);
    }
};

struct Function {
private:
    FctId m_id;
    // Assembly-compatible Mangled Name
    std::string m_name;
    std::vector<BasicBlock> m_blocks;   // block 0 is the entry

public:
    Function(FctId id, std::string name): m_id(id), m_name(name), m_blocks() {
        this->addBlock();
    }

    void addBlock() {
        BasicBlock b(this->m_blocks.size());
        this->m_blocks.push_back(std::move(b));
    }

    FctId id() const { return this->m_id; }
    const std::string& name() const { return this->m_name; }

    std::vector<BasicBlock>& blocks() { return this->m_blocks; }
    const std::vector<BasicBlock>& blocks() const { return this->m_blocks; }

    const BasicBlock& getBlock(BlockId bid) const {
        // Todo check bounds
        return this->m_blocks[bid];
    };

    BasicBlock& getBlock(BlockId bid) {
        // Todo check bounds
        return this->m_blocks[bid];
    };
};


// Translate AST to TAC
class SSAIR: public parser::AstVisitor<SSAIR, Operand> {
public:
    Function run(const parser::TranslationUnit& unit);

    Operand visitIntLiteral(const parser::IntLiteral& node);
    Operand visitFloatLiteral(const parser::FloatLiteral& node);
    Operand visitBoolLiteral(const parser::BoolLiteral& node);
    Operand visitStringLiteral(const parser::StringLiteral& node);
    Operand visitIdentifier(const parser::Identifier& node);
    Operand visitBinaryExpr(const parser::BinaryExpr& node);
    Operand visitAssign(const parser::Assign& node);
    Operand visitVarDecl(const parser::VarDecl& node);
    Operand visitExprStmt(const parser::ExprStmt& node);
    Operand visitTranslationUnit(const parser::TranslationUnit& node);

public:

    SSAIR():
        m_current_fct(0),
        m_current_block(0),
        m_fn(0, "blast_main")
    {}

    Function& currentFct() {
        return this->m_fn;
    }

    BasicBlock& currentBlock() {
        return this->currentFct().getBlock(this->m_current_block);
    }

private:
    FctId m_current_fct;
    BlockId m_current_block;
    Function m_fn;
};

} // namespace blast::core::ir