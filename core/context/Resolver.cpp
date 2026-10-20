#include <core/context/Resolver.hpp>
#include <core/Exception.hpp>
#include <cstdio>

namespace blast::core::context {


void ScopeResolver::run(const parser::TranslationUnit& unit) {
    this->m_current_scope = &this->m_ctx->mainScope();
    this->visit(&unit);
}


void ScopeResolver::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt: node.stmts()) {
        this->visit(stmt.get());
    }
}

void ScopeResolver::visitExprStmt(const parser::ExprStmt& node) {
    this->visit(node.expr());
}

void ScopeResolver::visitVarDecl(const parser::VarDecl& node) {
    // First let's see if we are declaring a unique symbol
    if (this->m_current_scope->hasSymbol(node.name())) {
        // Throw 'Variable already declared in this scope'
    }

    // Let's declare this new symbol in the scope
    Symbol* v = this->m_ctx->declare(*this->m_current_scope, node.name(), Symbol::Kind::Variable);
    // Let's bind this node to its resolved symbol
    this->m_ctx->setNodeSymbol(&node, v);
    
    if (node.hasType()) {
        this->visit(node.type());
    }

    if (node.hasInit()) {
        this->visit(node.init());
    }
}

void ScopeResolver::visitIdentifier(const parser::Identifier& node) {
    Symbol* s = this->m_current_scope->lookup(node.name());
    if (s == nullptr) {
        // Throw: Identifier not declared in this scope
        std::printf("[AF] Unknown Identifier %s\n", node.name().c_str());
    } else {
        this->m_ctx->setNodeSymbol(&node, s);
        std::printf("[AF] Visited identifier %s\n", node.name().c_str());
    }
}

void ScopeResolver::visitBinaryExpr(const parser::BinaryExpr& node) {
    this->visit(node.lhs());
    this->visit(node.rhs());
}

void ScopeResolver::visitAssign(const parser::Assign& node) {
    this->visit(node.value());
    this->visit(node.target());
}

void ScopeResolver::visitCallExpr(const parser::CallExpr& node) {
    this->visit(node.callee());
    for (const auto& arg: node.args()) {
        this->visit(arg.get());
    }

    // Now that this call has been visited, it has a symbol
    Symbol* s = this->m_ctx->getNodeSymbol(node.callee());
    // Set a CallExpr symbols as it's callee's symbol
    this->m_ctx->setNodeSymbol(&node, s);
}

void ScopeResolver::visitMemberExpr(const parser::MemberExpr& node) {
    this->visit(node.base());
    Symbol* base = this->m_ctx->getNodeSymbol(node.base());
    if (base == nullptr) {
        return;
    }
    if (base->kind() != Symbol::Kind::Module) {
        std::printf("[AF] '%.*s' is not a module\n", int(base->name().size()), base->name().data());
        return;
    }
    Symbol* member = base->moduleScope()->lookupLocal(node.member());
    if (member == nullptr) {
        std::printf("[AF] Unknown member %s of module %.*s\n", node.member().c_str(), int(base->name().size()), base->name().data());
    } else {
        this->m_ctx->setNodeSymbol(&node, member);
    }
}

void ScopeResolver::visitModuleDecl(const parser::ModuleDecl& node) {
    Symbol* module = this->m_ctx->declare(*this->m_current_scope, node.name(), Symbol::Kind::Module);
    if (module == nullptr) {
        std::printf("[AF] Module %s already declared in this scope\n", node.name().c_str());
        return;
    }
    this->m_ctx->setNodeSymbol(&node, module);
    // A module only sees its own names and Core's, whatever encloses it
    Scope& scope = this->m_ctx->newScope(Scope::Kind::Module, &this->m_ctx->coreScope());
    module->setModuleScope(&scope);

    Scope* last_scope = this->m_current_scope;
    this->m_current_scope = &scope;
    for (const auto& stmt: node.stmts()) {
        this->visit(stmt.get());
    }
    this->m_current_scope = last_scope;
}

void ScopeResolver::visitIfStmt(const parser::IfStmt& node) {
    this->visit(node.cond());
    this->visit(node.thenBranch());
    if (node.hasElse()) {
        this->visit(node.elseBranch());
    }
}

void ScopeResolver::visitWhileStmt(const parser::WhileStmt& node) {
    this->visit(node.cond());
    this->visit(node.body());
}

void ScopeResolver::visitBlock(const parser::Block& node) {
    // A block is a new scope
    Scope& block = this->m_ctx->newScope(Scope::Kind::Block, this->m_current_scope);
    Scope* save = this->m_current_scope;
    this->m_current_scope = &block;
    for (const auto& stmt: node.stmts()){
        this->visit(stmt.get());
    }
    if (node.hasReturn()) {
        this->visit(node.returnStmt());
    }
    this->m_current_scope = save;
}

void ScopeResolver::visitReturnStmt(const parser::ReturnStmt& node) {
    this->visit(node.expr());
}

void ScopeResolver::visitFctDecl(const parser::FctDecl& node) {
    // Extend the generic function of this scope if there is one, else create it
    Symbol* fcts = this->m_current_scope->lookupLocal(node.name());
    if (fcts == nullptr) {
        fcts = this->m_ctx->declare(*this->m_current_scope, node.name(), Symbol::Kind::Function);
        this->m_ctx->registerGeneric(node.name(), fcts);
    } else if (fcts->kind() != Symbol::Kind::Function) {
        std::printf("[AF] %s is already declared in this scope and is not a function\n", node.name().c_str());
        return;
    }
    this->m_ctx->setNodeSymbol(&node, fcts);
    // Signature is unknown until TypeResolver resolves the parameter types
    Method* method = this->m_ctx->registerMethod(fcts->function(), Method::Linkage::Internal, nullptr, &node);
    this->m_ctx->setNodeMethod(&node, method);
    // Now, enter the function's scope before
    Scope& fct_scope = this->m_ctx->newScope(Scope::Kind::Function, this->m_current_scope);
    Scope* last_scope = this->m_current_scope;
    this->m_current_scope = &fct_scope;

    for (const auto& param: node.params()) {
        // Declare the parameter in the new scope
        Symbol* p = this->m_ctx->declare(fct_scope, param->name(), Symbol::Kind::Parameter);
        // And register the symbol
        this->m_ctx->setNodeSymbol(param.get(), p);
        // Visit the param
        this->visit(param->type());
    }

    if (node.hasReturnType()) {
        this->visit(node.returnType());
    }

    this->visit(node.body());

    // Go back to the last scope
    this->m_current_scope = last_scope;
}

void TypeResolver::run(const parser::TranslationUnit& unit) {
    this->visit(&unit);
}

const Type* TypeResolver::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt: node.stmts()) {
        this->visit(stmt.get());
    }
    return nullptr;
}

const Type* TypeResolver::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

const Type* TypeResolver::visitVarDecl(const parser::VarDecl& node) {
    // We already ScopeResolver, so by this point this node should have a symbol for this node
    //
    Symbol* v = this->m_ctx->getNodeSymbol(&node);
    if (node.hasType()) {
        // Lookup the type hint symbol, it was already visited by ScopeResolver
        Symbol* ts = this->m_ctx->getNodeSymbol(node.type());
        Type* declared = ts ? ts->type() : nullptr;
        // If undeclared -> Throw ?
        if (declared) {
            //std::printf("[AF] Identifier %s is of type %s\n", node.name().c_str(), declared->name().c_str());
            // Set node's and variable's type
            this->m_ctx->setNodeType(&node, declared);
            v->setType(declared);
            return declared;
        } else {
            //std::printf("[AF] Identifier %s has undefined type\n", node.name().c_str());
        }
    }
    return nullptr;
}

// TODO: type float, bool and string literals once those types exist
const Type* TypeResolver::visitIntLiteral(const parser::IntLiteral& node) {
    Symbol* int_s = this->m_ctx->coreScope().lookupLocal("Int");
    const Type* type = int_s->type();
    this->m_ctx->setNodeType(&node, type);
    return type;
}

const Type* TypeResolver::visitIdentifier(const parser::Identifier& node) {
    Symbol* s = this->m_ctx->getNodeSymbol(&node);
    if (s == nullptr || (s->kind() != Symbol::Kind::Variable && s->kind() != Symbol::Kind::Parameter)) {
        return nullptr;
    }
    const Type* type = s->type();
    this->m_ctx->setNodeType(&node, type);
    return type;
}

const Type* TypeResolver::visitBinaryExpr(const parser::BinaryExpr& node) {
    const Type* lhs = this->visit(node.lhs());
    const Type* rhs = this->visit(node.rhs());
    if (lhs == nullptr || rhs == nullptr) {
        return nullptr;
    }
    if (lhs != rhs) {
        std::printf("[AF] Operator %s has operands of different types %s and %s\n", node.op().c_str(), lhs->name().c_str(), rhs->name().c_str());
        return nullptr;
    }
    // TODO: comparisons should return Bool, not the operand type
    this->m_ctx->setNodeType(&node, lhs);
    return lhs;
}

const Type* TypeResolver::visitAssign(const parser::Assign& node) {
    const Type* target = this->visit(node.target());
    const Type* value = this->visit(node.value());
    if (target == nullptr || value == nullptr) {
        return nullptr;
    }
    if (target != value) {
        std::printf("[AF] Cannot assign a %s to a %s\n", value->name().c_str(), target->name().c_str());
        return nullptr;
    }
    this->m_ctx->setNodeType(&node, target);
    return target;
}

const Type* TypeResolver::visitCallExpr(const parser::CallExpr& node) {
    this->visit(node.callee());
    for (const auto& arg: node.args()) {
        this->visit(arg.get());
    }
    // Now that this call has been visited, it has a symbol
    Symbol* s = this->m_ctx->getNodeSymbol(&node);
    // Methods have no type until dispatch exists, see the generic function's methods
    if (s == nullptr || s->type() == nullptr) {
        return nullptr;
    }

    std::printf("[AF] Calling fct '%.*s' ", int(s->name().size()), s->name().data());
    std::printf("with type %.*s \n", int(s->type()->name().size()), s->type()->name().data());

    return s->type();
}

const Type* TypeResolver::visitMemberExpr(const parser::MemberExpr& node) {
    this->visit(node.base());
    Symbol* s = this->m_ctx->getNodeSymbol(&node);
    return s ? s->type() : nullptr;
}

const Type* TypeResolver::visitModuleDecl(const parser::ModuleDecl& node) {
    for (const auto& stmt: node.stmts()) {
        this->visit(stmt.get());
    }
    return nullptr;
}

const Type* TypeResolver::visitBlock(const parser::Block& node) {
    // A type of a block if the type of its return statement
    const Type* type = this->nothingType();
    for (const auto& stmt: node.stmts()) {
        const Type* stmt_type = this->visit(stmt.get());
        // Only control flow can return, an expression's value is not a return
        if (stmt->kind() == parser::ASTNode::Kind::IfStmt || stmt->kind() == parser::ASTNode::Kind::WhileStmt) {
            type = this->joinTypes(type, stmt_type);
        }
    }
    if (node.hasReturn()) {
        type = this->joinTypes(type, this->visit(node.returnStmt()));
    }
    this->m_ctx->setNodeType(&node, type);
    return type;
}

const Type* TypeResolver::visitIfStmt(const parser::IfStmt& node) {
    this->visit(node.cond());
    const Type* type = this->visit(node.thenBranch());
    if (node.hasElse()) {
        type = this->joinTypes(type, this->visit(node.elseBranch()));
    }
    this->m_ctx->setNodeType(&node, type);
    return type;
}

const Type* TypeResolver::visitWhileStmt(const parser::WhileStmt& node) {
    this->visit(node.cond());
    const Type* type = this->visit(node.body());
    this->m_ctx->setNodeType(&node, type);
    return type;
}

const Type* TypeResolver::visitReturnStmt(const parser::ReturnStmt& node) {
    const Type* type = this->visit(node.expr());
    this->m_ctx->setNodeType(&node, type);
    return type;
}

const Type* TypeResolver::joinTypes(const Type* a, const Type* b) {
    if (a == nullptr || b == nullptr) {
        return nullptr;
    }
    if (a == this->nothingType()) {
        return b;
    }
    if (b == this->nothingType() || a == b) {
        return a;
    }
    std::printf("[AF] Paths return different types %s and %s\n", a->name().c_str(), b->name().c_str());
    return nullptr; // TODO: throw
}

const Type* TypeResolver::visitFctDecl(const parser::FctDecl& node) {
    // Argument types of the signature, in declaration order
    std::vector<const Type*> param_types;
    for (const auto& param: node.params()) {
        // Symbol of the annotation, bound by ScopeResolver
        Symbol* ts = this->m_ctx->getNodeSymbol(param->type());
        if (ts == nullptr || ts->type() == nullptr) {
            std::printf("[AF] Parameter %s of %s has an undefined type\n", param->name().c_str(), node.name().c_str());
            return nullptr; // TODO: throw
        }
        param_types.push_back(ts->type());
        // Give the parameter's own symbol its type so uses in the body are typed
        Symbol* ps = this->m_ctx->getNodeSymbol(param.get());
        ps->setType(ts->type());
        this->m_ctx->setNodeType(param.get(), ts->type());
    }

    // Type the body even when annotated, its statements need their types too
    const Type* body_type = this->visit(node.body());
    // The annotation wins, else the function has the type of its body
    const Type* return_type = body_type;
    if (node.hasReturnType()) {
        Symbol* rs = this->m_ctx->getNodeSymbol(node.returnType());
        return_type = rs ? rs->type() : nullptr;
    }
    if (return_type == nullptr) {
        std::printf("[AF] Function %s has an undefined return type\n", node.name().c_str());
        return nullptr; // TODO: throw
    }
    if (body_type != nullptr && body_type != this->nothingType() && body_type != return_type) {
        std::printf("[AF] Function %s returns %s but is declared %s\n", node.name().c_str(), body_type->name().c_str(), return_type->name().c_str());
        return nullptr; // TODO: throw
    }

    // Now we will construct the function's signature
    // 1: Get the arguments as a unique Tuple
    const TupleType* param_tuple = this->m_ctx->getTuple(param_types);
    // 2: Get the method the are implementing
    Method* method = this->m_ctx->getNodeMethod(&node);
    if (method == nullptr) {
        return nullptr; // TODO: throw
    }
    // 3: Loop over the methods (implementations) of the function to find duplicates
    for (const auto& other: this->m_ctx->getNodeSymbol(&node)->function()->methods()) {
        if (other.get() == method || other->type == nullptr) {
            continue;
        }
        if (other->type->args() == param_tuple) {
            std::printf("[AF] Function %s is already defined for %s\n", node.name().c_str(), param_tuple->name().c_str());
            return nullptr; // TODO: throw
        }
    }

    // 4: Build the method's signature
    method->type = this->m_ctx->getFunction(param_tuple, return_type);
    this->m_ctx->setNodeType(&node, method->type);
    return method->type;
}

}