#pragma once
#include <core/ir/IR.hpp>
#include <core/lexer/Lexer.hpp>
#include <core/parser/Ast.hpp>
#include <string>

// Debug rendering for every stage's data structures. Kept out of the stages
// themselves so a phase never links a printer it does not need, and so the
// formats stay consistent with one another.
namespace blast::core::utils {

// One token per line: "[KIND]\tvalue".
std::string dump(const lexer::Tokenizer& tokenizer);

// A node and its subtree as one s-expression: "(binary + (int 1) (int 2))".
std::string dump(const parser::ASTNode* node);

// A node and its subtree as a multi-line ASCII tree.
std::string dumpTree(const parser::ASTNode* node);

// A function as its blocks and instructions: "%0 = ADD %1, 2".
std::string dump(const ir::Function& fn);

} // namespace blast::core::utils
