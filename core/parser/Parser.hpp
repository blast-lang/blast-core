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

    // Every node reaches the tree through here, so this is where NodeIds are
    // handed out: ids come out dense over one parse, and a node kind added
    // later cannot be left unnumbered by accident.
    void push(std::unique_ptr<ASTNode> node) {
        node->setId(this->m_next_node_id++);
        this->m_parse_stack.push(std::move(node));
    }

    // Operator at the top of the operator stack, or the NONE sentinel when
    // empty, so callers can compare kinds without checking for emptiness first.
    const Token& topOperator() const {
        static const Token none{TokenKind::NONE, ""};
        return this->m_operator_stack.empty() ? none : this->m_operator_stack.top();
    }

    Token popOperator() {
        Token op = std::move(this->m_operator_stack.top());
        this->m_operator_stack.pop();
        return op;
    }

    void pushOperator(Token op) {
        this->m_operator_stack.push(std::move(op));
    }

    std::size_t operatorDepth() const { return this->m_operator_stack.size(); }


// Parsing
protected:

    // Parsing Statements
    void parseStmt();
    void parseDecl();
    void parseVarDecl();

    // Parsing Expressions
    void parseExpr();
    // A single operand: literal, identifier or a parenthesised expression.
    void parseOperand();
    void parseUnaryExpr();
    // Folds the top operator with the top two operands. Reads no tokens.
    void parseBinaryExpr();
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
    // Operator stack for Shaunting-Yard algorithm
    // https://en.wikipedia.org/wiki/Shunting_yard_algorithm
    std::stack<Token> m_operator_stack;
    // Next NodeId to hand out; see push().
    NodeId m_next_node_id = 0;
    // Root of the AST, owned by the parser and populated by buildAST().
    std::unique_ptr<TranslationUnit> m_root;
public:
    void buildAST();
    // Root of the parsed AST (null until buildAST() runs).
    const TranslationUnit* root() const { return this->m_root.get(); }
    // Number of nodes in the parsed tree; ids run over [0, nodeCount()), which
    // is what ASTContext sizes its side tables from.
    NodeId nodeCount() const { return this->m_next_node_id; }
};

} // namespace blast::core::parser