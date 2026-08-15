#pragma once

namespace blast::core::context {

// Everything one translation unit owns beyond its syntax tree: the types, the
// scope tree, and the semantic facts attached to nodes.
class AstContext {
public:
    AstContext();
    ~AstContext();

    // Will own the AST, the types and the scopes everything else points into.
    AstContext(const AstContext&) = delete;
    AstContext& operator=(const AstContext&) = delete;
};

} // namespace blast::core::context
