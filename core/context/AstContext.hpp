#pragma once

#include <core/context/Scope.hpp>
#include <core/parser/Ast.hpp>
#include <vector>

namespace blast::core::context {
// Everything one translation unit owns beyond its syntax tree: the types, the
// scope tree, and the semantic facts attached to nodes.
class AstContext {
public:
    AstContext();
    ~AstContext() = default;

    // Will own the AST, the types and the scopes everything else points into.
    AstContext(const AstContext&) = delete;
    AstContext& operator=(const AstContext&) = delete;

    // Where the resolver starts: the translation unit's own module scope.
    Scope& mainScope() { return this->m_main; }
    // The builtins. Every module glob-imports it, so lookups reach it without
    // naming it; this is for the few places that need the scope itself.
    Scope& coreScope() { return this->m_core; }

    // Size the side tables from SimpleParser::nodeCount(); setters grow anyway.
    void reserveNodes(parser::NodeId count);

    // What a node resolved to. An unnumbered node is ignored / reads back null.
    void setNodeSymbol(parser::NodeId id, Symbol* symbol);

    Symbol* nodeSymbol(parser::NodeId id);

    // Same, for the type deduced for a node.
    void setNodeType(parser::NodeId id, const Type* type);
    const Type* nodeType(parser::NodeId id) const;

private:
    // Core containing builtins symbols
    Scope m_core;
    // Translation unit to be contextualized
    Scope m_main;
    // Map nodes (id) to their deduced types
    std::vector<const Type*> m_node_types;
    // Same with symbols
    std::vector<Symbol*> m_node_symbols;
    // Unique set of Types, indexed by TypeId
    std::vector<std::unique_ptr<Type>> m_types;
};

} // namespace blast::core::context
