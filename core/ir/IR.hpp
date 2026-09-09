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
using ValueId = uint32_t;
using FctId = uint32_t;

struct Type {
    enum class Kind : std::uint8_t {
        INT,
        UINT,
        FLOAT,
        PTR,
        VOID
    };

    enum class Width : std::uint8_t {
        W1,
        W8,
        W16,
        W32,
        W64,
        W128
    };

    Kind  m_kind;
    Width m_width;

    constexpr bool operator==(const Type&) const = default;
};

constexpr unsigned bits(Type::Width w) {
    switch (w) {
        case Type::Width::W1:   return 1;
        case Type::Width::W8:   return 8;
        case Type::Width::W16:  return 16;
        case Type::Width::W32:  return 32;
        case Type::Width::W64:  return 64;
        case Type::Width::W128: return 128;
    }
    return 0;
}

constexpr unsigned bits(Type t) {
    return bits(t.m_width);
}

constexpr Type INT(Type::Width w) {
    return { Type::Kind::INT, w };
}

constexpr Type UINT(Type::Width w) {
    return { Type::Kind::UINT, w };
}

constexpr Type FLOAT(Type::Width w) {
    return { Type::Kind::FLOAT, w };
}

constexpr Type PTR() {
    return { Type::Kind::PTR, Type::Width::W64 };
}

// Width is meaningless here, but keeping the pair total avoids a third state.
constexpr Type VOID() {
    return { Type::Kind::VOID, Type::Width::W1 };
}

constexpr bool isInt(Type t) {
    return t.m_kind == Type::Kind::INT || t.m_kind == Type::Kind::UINT;
}

constexpr bool isSigned(Type t) {
    return t.m_kind == Type::Kind::INT;
}

constexpr bool isFloat(Type t) {
    return t.m_kind == Type::Kind::FLOAT;
}

struct Lireral {
    union {
        bool m_i1;
        std::int8_t m_i8;
        std::int16_t m_i16;
        std::int32_t m_i32;
        std::int64_t m_i64;
        std::uint8_t m_ui8;
        std::uint16_t m_ui16;
        std::uint32_t m_ui32;
        std::uint64_t m_ui64;
        float m_f32;
        double m_f64;
    };
};


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
        Lireral  m_lit;     // LITERAL
    };
};

inline Operand NONE() {
    return {
        .m_kind = Operand::Kind::NONE,
        .m_type = VOID(),
        .m_lit = { .m_i64 = 0 }
    };
}

inline Operand REGISTER(ValueId v, Type t = INT(Type::Width::W64)) {
    return {
        .m_kind = Operand::Kind::REGISTER,
        .m_type = t,
        .m_value = v
    };
}


inline Operand LITERAL(bool v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = INT(Type::Width::W1),
        .m_lit = { .m_i1 = static_cast<bool>(v) }
    };
}

inline Operand LITERAL(std::int8_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = INT(Type::Width::W8),
        .m_lit = { .m_i8 = v }
    };
}

inline Operand LITERAL(std::int16_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = INT(Type::Width::W16),
        .m_lit = { .m_i16 = v }
    };
}

inline Operand LITERAL(std::int32_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = INT(Type::Width::W32),
        .m_lit = { .m_i32 = v }
    };
}

inline Operand LITERAL(std::int64_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = INT(Type::Width::W64),
        .m_lit = { .m_i64 = v }
    };
}

inline Operand LITERAL(std::uint8_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = UINT(Type::Width::W8),
        .m_lit = { .m_ui8 = v }
    };
}

inline Operand LITERAL(std::uint16_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = UINT(Type::Width::W16),
        .m_lit = { .m_ui16 = v }
    };
}

inline Operand LITERAL(std::uint32_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = UINT(Type::Width::W32),
        .m_lit = { .m_ui32 = v }
    };
}

inline Operand LITERAL(std::uint64_t v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = UINT(Type::Width::W64),
        .m_lit = { .m_ui64 = v }
    };
}

inline Operand LITERAL(float v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = FLOAT(Type::Width::W32),
        .m_lit = { .m_f32 = v }
    };
}

inline Operand LITERAL(double v) {
    return {
        .m_kind = Operand::Kind::LITERAL,
        .m_type = FLOAT(Type::Width::W64),
        .m_lit = { .m_f64 = v }
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

struct Phi {
    Operand m_result;
    std::vector<std::pair<BlockId, Operand>> m_incomings;
};

class BasicBlock {
private:
    BlockId m_id;
    std::string m_label;
    // Block inputs, one per value live across an incoming edge
    std::vector<Phi>         m_phis;
    // Set of instructions in the block
    std::vector<Instruction> m_instrs;
    // Block parents to get Control flow Graph structure
    std::vector<BlockId>     m_preds;

public:
    BasicBlock(BlockId id, std::string label):
        m_id(id), m_label(std::move(label)), m_phis(), m_instrs(), m_preds()
    {}

    BlockId id() const { return this->m_id; }
    const std::string& label() const { return this->m_label; }

    std::vector<Phi>& phis() { return this->m_phis; }
    const std::vector<Phi>& phis() const { return this->m_phis; }

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
    // Function arguments, live on entry
    std::vector<Operand> m_args;
    Type m_ret;
    ValueId m_next_value;

public:
    Function(FctId id, std::string name, Type ret = VOID()):
        m_id(id), m_name(name), m_blocks(), m_args(), m_ret(ret), m_next_value(0)
    {
        this->addBlock("entry");
    }

    void addBlock(std::string label) {
        BasicBlock b(this->m_blocks.size(), std::move(label));
        this->m_blocks.push_back(std::move(b));
    }

    FctId id() const { return this->m_id; }
    const std::string& name() const { return this->m_name; }

    std::vector<BasicBlock>& blocks() { return this->m_blocks; }
    const std::vector<BasicBlock>& blocks() const { return this->m_blocks; }

    Type ret() const { return this->m_ret; }
    void setRet(Type t) { this->m_ret = t; }

    std::vector<Operand>& args() { return this->m_args; }
    const std::vector<Operand>& args() const { return this->m_args; }

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

    Operand addArg(Type t) {
        const Operand a = REGISTER(this->newValue(), t);
        this->m_args.push_back(a);
        return a;
    }

    // Incomings are filled by the caller, through getBlock(bid).phis()
    Operand addPhi(BlockId bid, Type t) {
        const Operand result = REGISTER(this->newValue(), t);
        this->getBlock(bid).phis().push_back(Phi{ result, {} });
        return result;
    }

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

    void addFct(std::string name, Type ret = VOID()) {
        this->m_fcts.push_back(Function(this->m_fcts.size(), name, ret));
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
    const Module& run(const parser::TranslationUnit& unit);

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