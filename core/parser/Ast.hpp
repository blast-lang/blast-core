#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace blast::core::parser {

// Dense index of a node within its own tree, handed out by
// context::ASTContext::numberNodes() once parsing is done. Semantic facts
// (symbols, types, ...) are kept in side tables indexed by this, so the AST
// itself stays a record of syntax and nothing else.
using NodeId = std::uint32_t;

// Id of a node that has not been numbered yet.
inline constexpr NodeId INVALID_NODE_ID = static_cast<NodeId>(-1);

// AST node hierarchy for BLAST.
//
// Every node carries a Kind tag in the base class so AstVisitor can dispatch
// on it without relying on C++ RTTI -- the build is compiled with -fno-rtti.
// Kinds are grouped by structure and kept contiguous within each group
// (Expr, Stmt, Decl), so classifying a node against a whole group stays a
// range check on the enum. Keep new kinds inside their group.
class ASTNode {
public:
    enum class Kind {
        // --- Expressions ---
        IntLiteral,
        FloatLiteral,
        BoolLiteral,
        StringLiteral,
        NothingLiteral,
        Identifier,
        UnaryExpr,
        BinaryExpr,
        Assign,
        // --- Declarations (an expression, as in Julia; keep last in the expr range) ---
        VarDecl,
        // --- Statements ---
        ExprStmt,
        IfStmt,
        ContinueStmt,
        Block,
        // --- Root ---
        TranslationUnit,
    };

    explicit ASTNode(Kind kind) : m_kind(kind) {}
    virtual ~ASTNode() = default;

    // Nodes own their children through unique_ptr; copying would need a deep
    // clone we don't want implicitly.
    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    Kind kind() const { return m_kind; }

    // Index into the side tables ASTContext keeps for this tree. The parser
    // stamps it as the node is pushed, so ids are dense and unique within one
    // parse; a node built by hand outside a parser keeps INVALID_NODE_ID and
    // simply has no side-table entry.
    NodeId id() const { return m_id; }
    void setId(NodeId id) { m_id = id; }

private:
    const Kind m_kind;
    NodeId m_id = INVALID_NODE_ID;
};

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------
class Expr: public ASTNode {
public:
    using ASTNode::ASTNode;
};

class IntLiteral: public Expr {
public:
    explicit IntLiteral(std::int64_t value): Expr(Kind::IntLiteral), m_value(value) {}
    std::int64_t value() const { return m_value; }

private:
    std::int64_t m_value;
};

class FloatLiteral: public Expr {
public:
    explicit FloatLiteral(double value): Expr(Kind::FloatLiteral), m_value(value) {}
    double value() const { return m_value; }

private:
    double m_value;
};

class BoolLiteral: public Expr {
public:
    explicit BoolLiteral(bool value): Expr(Kind::BoolLiteral), m_value(value) {}
    bool value() const { return m_value; }

private:
    bool m_value;
};

class StringLiteral: public Expr {
public:
    explicit StringLiteral(std::string value): Expr(Kind::StringLiteral), m_value(std::move(value)) {}
    const std::string& value() const { return m_value; }

private:
    std::string m_value;
};

class Identifier: public Expr {
public:
    explicit Identifier(std::string name): Expr(Kind::Identifier), m_name(std::move(name)) {}
    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

class UnaryExpr: public Expr {
public:
    UnaryExpr(std::string op, std::unique_ptr<Expr> operand): 
        Expr(Kind::UnaryExpr),
        m_op(std::move(op)),
        m_operand(std::move(operand))
    {}
    const std::string& op() const { return m_op; }
    const Expr* operand() const { return m_operand.get(); }

private:
    std::string m_op;
    std::unique_ptr<Expr> m_operand;
};

class BinaryExpr: public Expr {
public:
    BinaryExpr(std::string op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs): 
        Expr(Kind::BinaryExpr), 
        m_op(std::move(op)),
        m_lhs(std::move(lhs)), 
        m_rhs(std::move(rhs)) 
    {}
    const std::string& op() const { return m_op; }
    const Expr* lhs() const { return m_lhs.get(); }
    const Expr* rhs() const { return m_rhs.get(); }

private:
    std::string m_op;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------
class Stmt: public ASTNode {
public:
    using ASTNode::ASTNode;
};

// function call, assignments
class ExprStmt : public Stmt {
public:
    explicit ExprStmt(std::unique_ptr<Expr> expr): Stmt(Kind::ExprStmt), m_expr(std::move(expr)) {}
    const Expr* expr() const { return m_expr.get(); }
private:
    std::unique_ptr<Expr> m_expr;
};

// Assignment 'target = value'. Unlike BinaryExpr the operands are not symmetric:
// target must be an assignable location (lvalue), checked in semantic analysis.
// As in Julia it is an expression, so it nests and chains ('a = b = c').
class Assign: public Expr {
public:
    Assign(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs):
        Expr(Kind::Assign),
        m_lhs(std::move(lhs)),
        m_rhs(std::move(rhs))
    {}
    const Expr* target() const { return this->m_lhs.get(); }
    const Expr* value() const { return this->m_rhs.get(); }

private:
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class Block: public Stmt {
public:
    Block() : Stmt(Kind::Block) {}
    void add(std::unique_ptr<Stmt> stmt) { m_stmts.push_back(std::move(stmt)); }
    const std::vector<std::unique_ptr<Stmt>>& stmts() const { return m_stmts; }

private:
    std::vector<std::unique_ptr<Stmt>> m_stmts;
};

/*
class IfStmt: public Stmt {
public:
    // elseBranch may be null.
    IfStmt(std::unique_ptr<Expr> cond,
           std::unique_ptr<Block> thenBranch,
           std::unique_ptr<Stmt> elseBranch): // nullable
                Stmt(Kind::IfStmt), 
                m_cond(std::move(cond)),
                m_then(std::move(thenBranch)), 
                m_else(std::move(elseBranch))
            {}
    const Expr* cond() const { return m_cond.get(); }
    const Block* thenBranch() const { return m_then.get(); }

    bool hasElse() const { return m_else != nullptr; }
    const Stmt* elseBranch() const { return m_else.get(); }   // null unless hasElse()


private:
    std::unique_ptr<Expr> m_cond;
    std::unique_ptr<Block> m_then;
    std::unique_ptr<Stmt> m_else;
};
*/

class ContinueStmt : public Stmt {
public:
    ContinueStmt() : Stmt(Kind::ContinueStmt) {}
};

// ---------------------------------------------------------------------------
// Declarations (Exprs)
// ---------------------------------------------------------------------------
class Decl: public Expr {
public:
    using Expr::Expr;
    // Every declaration binds a name into a scope.
    virtual const std::string& name() const = 0;
};

// A variable declaration, e.g.  x::Int = 5
//   name is required; type (the annotation after '::') and init (after '=')
//   are each optional and null when absent.
class VarDecl: public Decl {
public:
    VarDecl(std::string name,
            std::unique_ptr<Expr> type,
            std::unique_ptr<Expr> init
        ): 
        Decl(Kind::VarDecl), m_name(std::move(name)), m_type(std::move(type)), m_init(std::move(init)) {}

    const std::string& name() const override { return m_name; }

    bool hasType() const { return m_type != nullptr; }
    const Expr* type() const { return m_type.get(); }   // null unless hasType()

    bool hasInit() const { return m_init != nullptr; }
    const Expr* init() const { return m_init.get(); }   // null unless hasInit()

private:
    std::string m_name;
    std::unique_ptr<Expr> m_type;   // Identifier now; TypeExpr later (Vector{Int})
    std::unique_ptr<Expr> m_init;
};

// ---------------------------------------------------------------------------
// Root
// ---------------------------------------------------------------------------
class TranslationUnit : public ASTNode {
public:
    TranslationUnit() : ASTNode(Kind::TranslationUnit) {}
    void add(std::unique_ptr<Stmt> stmt) { m_stmts.push_back(std::move(stmt)); }
    const std::vector<std::unique_ptr<Stmt>>& stmts() const { return m_stmts; }

private:
    std::vector<std::unique_ptr<Stmt>> m_stmts;
};


} // namespace blast::core::parser
