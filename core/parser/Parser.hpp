#pragma once
#include <stack>
#include <core/parser/Ast.hpp>
#include <core/lexer/Lexer.hpp>

namespace blast::core::parser {

class SimpleParser {
public:
    using Tokenizer = blast::core::lexer::Tokenizer;
    using Token     = Tokenizer::Token;
    using TokenKind = Tokenizer::TokenKind;

    explicit SimpleParser(const Tokenizer& tokenizer): m_tokenizer(tokenizer), m_pos(0) {}

// Utils
private:
    // Token at the cursor
    const Token& currentToken() const { return tokenAt(m_pos); }
    // One-token lookahead
    const Token& nextToken() const { return tokenAt(m_pos + 1); }

    // Consume the current token and move to the next, returning the one consumed.
    // Stops advancing past the end so currentToken() stays on the NONE sentinel.
    const Token& advance() {
        const Token& consumed = currentToken();
        if (m_pos <  this->m_tokenizer.tokens().size()) {
            ++m_pos;
        }
        return consumed;
    }

    const Token& tokenAt(std::size_t pos) const {
        static const Token eof{TokenKind::NONE, ""};
        const auto& tokens =  this->m_tokenizer.tokens();
        return pos < tokens.size() ? tokens[pos] : eof;
    }

    ASTNode* peek() { return  this->m_parse_stack.empty() ? nullptr : this->m_parse_stack.top().get(); }

    std::unique_ptr<ASTNode> pop() {
        auto node = std::move(this->m_parse_stack.top());
        this->m_parse_stack.pop();
        return node;
    }

    // Pop and downcast to a known node type. The cast is unchecked: the caller
    // is the one that pushed the node, so it already knows what is on top.
    template <typename T>
    std::unique_ptr<T> popAs() {
        return std::unique_ptr<T>(static_cast<T*>(this->pop().release()));
    }

    void push(std::unique_ptr<ASTNode> node) {
        this->m_parse_stack.push(std::move(node));
    }


// Parsing
protected:

    // Parsing Statements
    void parseStmt();
    void parseDecl();
    void parseVarDecl();

    // Parsing Expressions
    void parseExpr();
    void parseLiteral();
    void parserIdentifier();

    // Root
    void parseTranslationUnit();


protected:
    const Tokenizer& m_tokenizer;
    // Cursor into m_tokenizer.tokens(); the read position lives in the parser,
    // not the tokenizer, so the token stream stays reusable.
    std::size_t m_pos;
    // Parsing stack
    std::stack<std::unique_ptr<ASTNode>> m_parse_stack;
    // Root of the AST, owned by the parser and populated by buildAST().
    std::unique_ptr<TranslationUnit> m_root;
public:
    void buildAST();
    // Root of the parsed AST (null until buildAST() runs).
    const TranslationUnit* root() const { return this->m_root.get(); }
};

} // namespace blast::core::parser