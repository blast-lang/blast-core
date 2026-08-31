// https://en.wikipedia.org/wiki/Dominator_(graph_theory)
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <variant>

#include <core/Exception.hpp>
#include <core/context/ASTContext.hpp>
#include <core/parser/AstVisitor.hpp>


// SSA (Static Single Assignment) Form
namespace blast::core::ir {


using BlockId = uint32_t;
using ValueId  = uint32_t;
using FctId  = uint32_t;

enum class Type {
    I1,
    I8,
    I16,
    I32,
    I64,

    F32,
    F64,

    PTR,
    VOID
};

inline bool isInt(Type t) {
    return t >= Type::I1 && t <= Type::I64;
}

inline bool isFloat(Type t) {
    return t >= Type::F32 && t <= Type::F64;
}

// Labels: prefix '%' for local and '@' for global
struct Operand {
    enum class Kind {
        NONE,
        BLOCK,
        REGISTER,
        LITERAL,
    };

    Kind m_kind;
    Type m_type;
    union {
        ValueId  m_value;   // REGISTER
        BlockId  m_block;   // BLOCK
        int64_t  m_i64;     // LITERAL
    };
};

inline Operand NONE() {
    return {
        .m_kind = Operand::Kind::NONE,
        .m_type = Type::VOID,
        .m_i64 = 0
    };
}

inline Operand REGISTER(ValueId v) {
    return {
        .m_kind = Operand::Kind::REGISTER,
        .m_type = Type::I64,
        .m_value = v
    };
}

inline Operand INT_LIT(std::int64_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = Type::I64,
        .m_i64 = v
    };
}


// Order is load-bearing: isComparison() and isTerminator() are range checks.
enum class Opcode: std::uint8_t {
    // Arithmetic: result = lhs op rhs
    ADD, SUB, MUL, DIV,
    NEG,  // unary '-'
    // Comparisons: LT .. NE
    LT, LE, GT, GE, EQ, NE,
    COPY, CALL,
    // Terminators: BR .. end
    BR, CBR, RET,
};

constexpr bool isTerminator(Opcode op) {
    return op >= Opcode::BR;
}

constexpr bool isComparison(Opcode op) {
    return op >= Opcode::LT && op <= Opcode::NE;
}

constexpr bool definesValue(Opcode op) {
    return !isTerminator(op);
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

    // result is minted by the owning Function: value ids are unique per
    // function, not per block. Prefer Function::addInstruction over this.
    Operand addInstruction(Operand result, Operand lhs, Operand rhs, Opcode op) {
        this->m_instrs.push_back(Instruction(result, lhs, rhs, op));
        return result;
    }
};

class Function {
private:
    FctId m_id;
    // Assembly-compatible Mangled Name
    std::string m_name;
    std::vector<BasicBlock> m_blocks;   // block 0 is the entry
    ValueId m_next_value;

public:
    Function(FctId id, std::string name):
        m_id(id), m_name(name), m_blocks(), m_next_value(0)
    {
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
        if (bid >= this->m_blocks.size())
            throw CodegenError("[Function] Unknown block id " + std::to_string(bid)
                               + " in '" + this->m_name + "'");
        return this->m_blocks[bid];
    };

    BasicBlock& getBlock(BlockId bid) {
        const Function& self = *this;
        return const_cast<BasicBlock&>(self.getBlock(bid));
    };

    ValueId newValue() { return this->m_next_value++; }

    Operand addInstruction(BlockId bid, Operand lhs, Operand rhs, Opcode op) {
        const Operand result = definesValue(op) ? REGISTER(this->newValue()) : NONE();
        return this->getBlock(bid).addInstruction(result, lhs, rhs, op);
    }
};

class Module {
private:
    std::vector<Function> m_fcts;

public:

    Module() = default;

    void addFct(std::string name) {
        this->m_fcts.push_back(Function(this->m_fcts.size(), name));
    }

    std::vector<Function>& fcts() { return this->m_fcts; }
    const std::vector<Function>& fcts() const { return this->m_fcts; }

    Function& getFct(FctId fid) {
        if (fid >= this->m_fcts.size())
            throw CodegenError("[Module] Unknown function id " + std::to_string(fid));
        return this->m_fcts[fid];
    };
};


// std::pair has no std::hash. Both halves are uint32, so packing them into a
// 64-bit size_t is exact: distinct keys never collide.
struct SymbolBlockHash {
    std::size_t operator()(const std::pair<context::SymbolId, BlockId>& k) const noexcept {
        return (static_cast<std::size_t>(k.first) << 32) | k.second;
    }
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

    SSAIR(context::ASTContext& ctx):
        m_current_fct(0),
        m_current_block(0),
        m_main(),
        m_ctx(&ctx),
        m_lko()
    {
        this->m_main.addFct("blast_main");
    }

    Function& currentFct() {
        return this->m_main.getFct(this->m_current_fct);
    }

    BasicBlock& currentBlock() {
        return this->currentFct().getBlock(this->m_current_block);
    }

    Operand addInstruction(Operand lhs, Operand rhs, Opcode op) {
        return this->currentFct().addInstruction(this->m_current_block, lhs, rhs, op);
    }

    void setLKO(context::Symbol* s, const Operand& o) {
        if (s) {
            this->m_lko.insert_or_assign(
                std::make_pair(s->id(), this->currentBlock().id()), o
            );
        }
    }

    Operand getLKO(context::Symbol* s) {
        if (s) {
            const auto p = std::make_pair(s->id(), this->currentBlock().id());
            auto it = this->m_lko.find(p);
            if (it == this->m_lko.end()) {
                return NONE();
            }
            return it->second;
        }
        return NONE();
    }

private:
    FctId m_current_fct;
    BlockId m_current_block;
    Module m_main;
    context::ASTContext* m_ctx;
    // Map the Last Known Operand (register) attributed to a given variable
    std::unordered_map<std::pair<context::SymbolId, BlockId>, Operand, SymbolBlockHash> m_lko;
};

} // namespace blast::core::ir