#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace blast::core::parser {

// AST node hierarchy for BLAST.
//
// Every node carries a Kind tag in the base class so we can dispatch with
// isa<>/dyn_cast<> (see below) without relying on C++ RTTI -- the build is
// compiled with -fno-rtti. Kinds are grouped by structure (Expr vs Stmt) and
// kept contiguous so the intermediate bases can classof() with a range check.
class ASTNode {
public:
    enum class Kind {
        // --- Expressions ---
        IntLiteral,
        FloatLiteral,
        BoolLiteral,
        StringLiteral,
        Identifier,
        UnaryExpr,
        BinaryExpr,
        Assign,
        // --- Statements ---
        ExprStmt,
        IfStmt,
        ContinueStmt,
        Block,
        // --- Declarations (a kind of statement; keep last in the stmt range) ---
        VarDecl,
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

private:
    const Kind m_kind;
};

// isa<T>(node)      -> is node (dynamically) a T?
// dyn_cast<T>(node) -> node as T*, or nullptr if it isn't one.
// Both rely on each concrete/intermediate class exposing a static classof().
template <typename To, typename From>
bool isa(const From* node) {
    return node != nullptr && To::classof(node);
}

template <typename To, typename From>
To* dyn_cast(From* node) {
    return isa<To>(node) ? static_cast<To*>(node) : nullptr;
}

template <typename To, typename From>
const To* dyn_cast(const From* node) {
    return isa<To>(node) ? static_cast<const To*>(node) : nullptr;
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------
class Expr: public ASTNode {
public:
    using ASTNode::ASTNode;
    static bool classof(const ASTNode* n) {
        return n->kind() >= Kind::IntLiteral && n->kind() <= Kind::Assign;
    }
};

class IntLiteral: public Expr {
public:
    explicit IntLiteral(std::int64_t value): Expr(Kind::IntLiteral), m_value(value) {}
    std::int64_t value() const { return m_value; }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::IntLiteral; }

private:
    std::int64_t m_value;
};

class FloatLiteral: public Expr {
public:
    explicit FloatLiteral(double value): Expr(Kind::FloatLiteral), m_value(value) {}
    double value() const { return m_value; }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::FloatLiteral; }

private:
    double m_value;
};

class BoolLiteral: public Expr {
public:
    explicit BoolLiteral(bool value): Expr(Kind::BoolLiteral), m_value(value) {}
    bool value() const { return m_value; }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::BoolLiteral; }

private:
    bool m_value;
};

class StringLiteral: public Expr {
public:
    explicit StringLiteral(std::string value): Expr(Kind::StringLiteral), m_value(std::move(value)) {}
    const std::string& value() const { return m_value; }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::StringLiteral; }

private:
    std::string m_value;
};

class Identifier: public Expr {
public:
    explicit Identifier(std::string name): Expr(Kind::Identifier), m_name(std::move(name)) {}
    const std::string& name() const { return m_name; }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::Identifier; }

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
    static bool classof(const ASTNode* n) { return n->kind() == Kind::UnaryExpr; }

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
    static bool classof(const ASTNode* n) { return n->kind() == Kind::BinaryExpr; }

private:
    std::string m_op;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

// Assignment 'target = value'. Unlike BinaryExpr the operands are not symmetric:
// target must be an assignable location (lvalue), checked in semantic analysis,
// not here. It is an Expr (right-associative, low precedence) so 'a = b = 5'
// chains as 'a = (b = 5)'.
class Assign: public Expr {
public:
    Assign(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value):
        Expr(Kind::Assign),
        m_target(std::move(target)),
        m_value(std::move(value))
    {}
    const Expr* target() const { return m_target.get(); }
    const Expr* value() const { return m_value.get(); }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::Assign; }

private:
    std::unique_ptr<Expr> m_target;
    std::unique_ptr<Expr> m_value;
};

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------
class Stmt: public ASTNode {
public:
    using ASTNode::ASTNode;
    static bool classof(const ASTNode* n) {
        // Spans the plain statements through the declarations (VarDecl last).
        return n->kind() >= Kind::ExprStmt && n->kind() <= Kind::VarDecl;
    }
};

class ExprStmt : public Stmt {
public:
    explicit ExprStmt(std::unique_ptr<Expr> expr)
        : Stmt(Kind::ExprStmt), m_expr(std::move(expr)) {}
    const Expr* expr() const { return m_expr.get(); }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::ExprStmt; }

private:
    std::unique_ptr<Expr> m_expr;
};

class Block: public Stmt {
public:
    Block() : Stmt(Kind::Block) {}
    void add(std::unique_ptr<Stmt> stmt) { m_stmts.push_back(std::move(stmt)); }
    const std::vector<std::unique_ptr<Stmt>>& stmts() const { return m_stmts; }
    static bool classof(const ASTNode* n) { return n->kind() == Kind::Block; }

private:
    std::vector<std::unique_ptr<Stmt>> m_stmts;
};

class IfStmt: public Stmt {
public:
    // else_branch may be null.
    IfStmt(std::unique_ptr<Expr> cond,
           std::unique_ptr<Block> then_branch,
           std::unique_ptr<Stmt> else_branch): // nullable
                Stmt(Kind::IfStmt), 
                m_cond(std::move(cond)),
                m_then(std::move(then_branch)), 
                m_else(std::move(else_branch))
            {}
    const Expr* cond() const { return m_cond.get(); }
    const Block* then_branch() const { return m_then.get(); }

    bool has_else() const { return m_else != nullptr; }
    const Stmt* else_branch() const { return m_else.get(); }   // null unless has_else()

    static bool classof(const ASTNode* n) { return n->kind() == Kind::IfStmt; }

private:
    std::unique_ptr<Expr> m_cond;
    std::unique_ptr<Block> m_then;
    std::unique_ptr<Stmt> m_else;
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt() : Stmt(Kind::ContinueStmt) {}
    static bool classof(const ASTNode* n) { return n->kind() == Kind::ContinueStmt; }
};

// ---------------------------------------------------------------------------
// Declarations (Stmts)
// ---------------------------------------------------------------------------
class Decl: public Stmt {
public:
    using Stmt::Stmt;
    static bool classof(const ASTNode* n) {
        // Sub-range of the statements; extend the upper bound as decl kinds grow.
        return n->kind() >= Kind::VarDecl && n->kind() <= Kind::VarDecl;
    }
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

    bool has_type() const { return m_type != nullptr; }
    const Expr* type() const { return m_type.get(); }   // null unless has_type()

    bool has_init() const { return m_init != nullptr; }
    const Expr* init() const { return m_init.get(); }   // null unless has_init()

    static bool classof(const ASTNode* n) { return n->kind() == Kind::VarDecl; }

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
    static bool classof(const ASTNode* n) { return n->kind() == Kind::TranslationUnit; }

private:
    std::vector<std::unique_ptr<Stmt>> m_stmts;
};



// Debug: render a node (and its subtree) as an s-expression-ish string.
std::string dump(const ASTNode* node);

// Debug: render a node (and its subtree) as a multi-line ASCII tree.
std::string dump_tree(const ASTNode* node);

} // namespace blast::core::parser
