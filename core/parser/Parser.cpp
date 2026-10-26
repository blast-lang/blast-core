#include "Parser.hpp"
#include <core/parser/Exception.hpp>

#include <iostream>

namespace blast::core::parser {

// Throws rather than defaulting to 0: a binding power of 0 would silently equal
// the empty-stack floor, so a new operator the lexer knows and the parser does
// not would misfold instead of being reported.
int bindingPower(const SimpleParser::Token& t) {
    switch (t.m_kind) {
        case SimpleParser::TokenKind::ASSIGN: return 1;   // low, right-assoc
        case SimpleParser::TokenKind::BIN_OP:             // +,* differentiated by value
            if (t.m_value == "*" ) return 20;
            if (t.m_value == "+" ) return 10;
            if (t.m_value == ">" ) return 5;
            if (t.m_value == "<" ) return 5;
            if (t.m_value == "<=" ) return 5;
            if (t.m_value == ">=" ) return 5;
            if (t.m_value == "==" ) return 5;
            if (t.m_value == "!=" ) return 5;
            throw UnknownOperator(t, 0);
        default:
            throw UnexpectedToken({SimpleParser::TokenKind::BIN_OP, SimpleParser::TokenKind::ASSIGN}, t, 0);
    }
}

void SimpleParser::parseStmt() {
    const SourceLocation start = this->currentToken().m_location;
    if (this->currentToken().m_kind == TokenKind::IDENDIFIER &&
        this->nextToken().m_kind == TokenKind::COLON_COLON) {
        this->parseVarDecl();
    } else if(this->currentToken().m_kind == TokenKind::IF_STMT) {
        this->parseIfStmt();
        // Return here as we do not expect ';' after an if stmt
        return;
    } else if (this->currentToken().m_kind == TokenKind::WHILE_STMT) {
        this->parseWhileStmt();
        // Return here as we do not expect ';' after a while stmt
        return;
    } else if (this->currentToken().m_kind == TokenKind::RETURN_STMT) {
        this->advance();
        this->parseExpr();
        this->push(std::make_unique<ReturnStmt>(this->popAs<Expr>()), start);
        if (this->currentToken().m_kind != TokenKind::ENDEXPR) {
            throw UnexpectedToken({TokenKind::ENDEXPR}, this->currentToken(), m_pos);
        }
        this->advance();
        return;
    } else if (this->currentToken().m_kind == TokenKind::FUNC_STMT) {
        this->parserFctDecl();
        // Return here as we do not expect ';' after a function body
        return;
    } else if (this->currentToken().m_kind == TokenKind::MODULE_STMT) {
        this->parseModuleDecl();
        // Return here as we do not expect ';' after a module body
        return;
    }
    else {
        this->parseExpr();
    }

    // Both branches leave an Expr on the stack (declarations are expressions,
    // as in Julia); at statement level its value is discarded.
    this->push(std::make_unique<ExprStmt>(this->popAs<Expr>()), start);

    // ';' terminates the statement.
    if (this->currentToken().m_kind != TokenKind::ENDEXPR) {
        throw UnexpectedToken({TokenKind::ENDEXPR}, this->currentToken(), m_pos);
    }
    this->advance();
    return;
}

void SimpleParser::parseDecl() {
    return;
}

// 'name :: Type' with an optional '= init'.
void SimpleParser::parseVarDecl() {
    this->parserIdentifier();
    auto name = this->popAs<Identifier>();

    if (this->currentToken().m_kind != TokenKind::COLON_COLON) {
        throw UnexpectedToken({TokenKind::COLON_COLON}, this->currentToken(), m_pos);
    }
    this->advance();

    // Type annotation: an identifier for now, a type expression later.
    this->parserIdentifier();
    auto type = this->popAs<Identifier>();

    // Initializer is optional: 'a::Int;' declares without initialising.
    std::unique_ptr<Expr> init = nullptr;
    if (this->currentToken().m_kind == TokenKind::ASSIGN) {
        this->advance();
        this->parseExpr();
        init = this->popAs<Expr>();
    }

    const SourceLocation start = name->location();
    this->push(std::make_unique<VarDecl>(name->name(), std::move(type), std::move(init)), start);
    return;
}


void SimpleParser::parseIfStmt() {
    const SourceLocation start = this->currentToken().m_location;
    // Parse the condition as expression
    this->advance();
    this->parseExpr();
    std::unique_ptr<Expr> cond = this->popAs<Expr>();
    std::unique_ptr<Block> if_block = nullptr;
    std::unique_ptr<Block> else_block = nullptr;
    // Now parse 'if' block content
    if (this->currentToken().m_kind == TokenKind::OPEN_CURLBRAC) {
        this->parseBlock();
        if_block = this->popAs<Block>();
        if (this->currentToken().m_kind == TokenKind::ELSE_STMT) {
            this->advance();
            this->parseBlock();
            else_block = this->popAs<Block>();
        }
    }
    // Push the if statement onto the stack
    this->push(std::make_unique<IfStmt>(std::move(cond), std::move(if_block), std::move(else_block)), start);
    return;
}

void SimpleParser::parseWhileStmt() {
    const SourceLocation start = this->currentToken().m_location;
    // Parse the condition as expression
    this->advance();
    this->parseExpr();
    std::unique_ptr<Expr> cond = this->popAs<Expr>();
    std::unique_ptr<Block> body = nullptr;
    if (this->currentToken().m_kind == TokenKind::OPEN_CURLBRAC) {
        this->parseBlock();
        body = this->popAs<Block>();
    } else {
        // TODO: throw
        throw UnexpectedToken({TokenKind::OPEN_CURLBRAC}, this->currentToken(), m_pos);
    }
    // Push the wile statement onto the stack
    this->push(std::make_unique<WhileStmt>(std::move(cond), std::move(body)), start);
    return;
}

void SimpleParser::parseForStmt() {

}

void SimpleParser::parseBlock() {
    const SourceLocation start = this->currentToken().m_location;
    std::unique_ptr<Block> block = std::make_unique<Block>();
    // Consume opening bracket
    this->advance();
    while(this->currentToken().m_kind != TokenKind::CLOSE_CURLBRAC) {
        this->parseStmt();
        if (this->peek()->kind() == ASTNode::Kind::ReturnStmt) {
            block->setReturn(this->popAs<ReturnStmt>());
        } else {
            block->add(this->popAs<Stmt>());
        }
    }
    // Consume closing bracket
    this->advance();
    // Push the block onto the stack
    this->push(std::move(block), start);
    return;
}


// Shunting-yard: operands go straight to the parse stack, operators wait on
// m_operator_stack until one that binds less tightly arrives and forces a fold.
void SimpleParser::parseExpr() {
    // Operators already on the stack belong to an enclosing expression -- a
    // parenthesised group must never fold past the ones its caller left behind.
    const std::size_t base = this->operatorDepth();

    while (true) {
        this->parseOperand();
        const Token op = this->currentToken();
        if (!(op.m_kind == TokenKind::BIN_OP || op.m_kind == TokenKind::ASSIGN)) {
            break;
        }

        // Left-assoc: an operator of equal precedence already on the stack folds first
        // so 8 + 9 + 10 -> (8 + 9) + 10. Right-assoc: only a strictly
        // tighter one folds, so a = b = c -> a = (b = c).
        const int bp = bindingPower(op);
        const bool right_assoc = (op.m_kind == TokenKind::ASSIGN);
        while (this->operatorDepth() > base &&
               (right_assoc ? bindingPower(this->topOperator()) >  bp
                            : bindingPower(this->topOperator()) >= bp)
        ) {
            this->parseBinaryExpr();
        }
        this->pushOperator(op);
        this->advance();
    }

    // End of the expression: fold what is left, down to our own floor.
    while (this->operatorDepth() > base) {
        this->parseBinaryExpr();
    }
    return;
}

// Exactly one operand, or the expression is malformed: parseBinaryExpr pops two
// of these, so failing to push one here would underflow the parse stack.
void SimpleParser::parseOperand() {
    if (this->currentToken().m_kind == TokenKind::IDENDIFIER) {
        this->parserIdentifier();
    } else if (
        this->currentToken().m_kind == TokenKind::INT_LIT ||
        this->currentToken().m_kind == TokenKind::FLOAT_LIT ||
        this->currentToken().m_kind == TokenKind::BOOL_LIT ||
        this->currentToken().m_kind == TokenKind::STR_LIT
    ) {
        this->parseLiteral();
        // Read a group inside (...)
    } else if (this->currentToken().m_kind == TokenKind::OPEN_PAR) {
        this->advance();
        // Fresh floor: the group process everything up to its ')'.
        this->parseExpr();
        if (this->currentToken().m_kind != TokenKind::CLOSE_PAR) {
            throw UnexpectedToken({TokenKind::CLOSE_PAR}, this->currentToken(), m_pos);
        }
        this->advance();
    } else {
        throw UnexpectedToken({TokenKind::INT_LIT, TokenKind::FLOAT_LIT, TokenKind::BOOL_LIT, TokenKind::STR_LIT, TokenKind::IDENDIFIER, TokenKind::OPEN_PAR}, this->currentToken(), m_pos);
    }

    while (this->currentToken().m_kind == TokenKind::OPEN_PAR ||
           this->currentToken().m_kind == TokenKind::DOT) {
        if (this->currentToken().m_kind == TokenKind::DOT) {
            this->parseMemberExpr();
        } else {
            this->parseCallExpr();
        }
    }
}

void SimpleParser::parseUnaryExpr() {

}

// Folds the operator on top of m_operator_stack with the two operands on top of
// the parse stack. Purely a stack operation -- it reads no tokens, so the caller
// that decided to fold now is the one that decided precedence.
void SimpleParser::parseBinaryExpr() {
    Token op  = this->popOperator();
    // rhs comes off first as it was pushed last
    auto  rhs = this->popAs<Expr>();
    auto  lhs = this->popAs<Expr>();
    const SourceLocation start = lhs->location();

    if (op.m_kind == TokenKind::ASSIGN) {
        this->push(std::make_unique<Assign>(std::move(lhs), std::move(rhs)), start);
    } else {
        this->push(std::make_unique<BinaryExpr>(
            std::move(op.m_value),
            std::move(lhs),
            std::move(rhs)
        ), start);
    }
    return;
}


void SimpleParser::parseModuleDecl() {
    const SourceLocation start = this->currentToken().m_location;
    // skip 'module'
    this->advance();
    this->parserIdentifier();
    auto name = this->popAs<Identifier>();
    if (this->currentToken().m_kind != TokenKind::OPEN_CURLBRAC) {
        throw UnexpectedToken({TokenKind::OPEN_CURLBRAC}, this->currentToken(), m_pos);
    }
    this->advance();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (this->currentToken().m_kind != TokenKind::CLOSE_CURLBRAC) {
        if (this->currentToken().m_kind == TokenKind::NONE) {
            throw UnexpectedToken({TokenKind::CLOSE_CURLBRAC}, this->currentToken(), m_pos);
        }
        this->parseStmt();
        stmts.push_back(this->popAs<Stmt>());
    }
    this->advance();
    this->push(std::make_unique<ModuleDecl>(name->name(), std::move(stmts)), start);
}

void SimpleParser::parseMemberExpr() {
    auto base = this->popAs<Expr>();
    const SourceLocation start = base->location();
    // skip '.'
    this->advance();
    const Token& member = this->currentToken();
    if (member.m_kind != TokenKind::IDENDIFIER && member.m_kind != TokenKind::BIN_OP) {
        throw UnexpectedToken({TokenKind::IDENDIFIER, TokenKind::BIN_OP}, member, m_pos);
    }
    std::string name = member.m_value;
    this->advance();
    this->push(std::make_unique<MemberExpr>(std::move(base), std::move(name)), start);
}

void SimpleParser::parseCallExpr() {
    auto callee = this->popAs<Expr>();
    const SourceLocation start = callee->location();
    // skip '('
    this->advance();
    std::vector<std::unique_ptr<Expr>> args;
    // Skip empty call
    if (this->currentToken().m_kind != TokenKind::CLOSE_PAR) {
        bool cont = true;
        while(cont) {
            this->parseExpr();
            args.push_back(this->popAs<Expr>());
            if (this->currentToken().m_kind == TokenKind::COMMA) {
                this->advance();
            } else {
                cont = false;
            }
        }
        if (this->currentToken().m_kind != TokenKind::CLOSE_PAR) {
            throw UnexpectedToken({TokenKind::CLOSE_PAR}, this->currentToken(), m_pos);
        }
    }
    this->advance();
    this->push(std::make_unique<CallExpr>(std::move(callee), std::move(args)), start);
}

void SimpleParser::parserFctDecl() {
    const SourceLocation start = this->currentToken().m_location;
    this->advance();
    this->parserIdentifier();
    auto name = this->popAs<Identifier>();

    if (this->currentToken().m_kind != TokenKind::OPEN_PAR) {
        // TODO: Throw
        throw UnexpectedToken({TokenKind::OPEN_PAR}, this->currentToken(), m_pos);
    }
    this->advance();

    std::vector<std::unique_ptr<VarDecl>> args;
    while (this->currentToken().m_kind != TokenKind::CLOSE_PAR) {
        this->parseVarDecl();
        args.push_back(this->popAs<VarDecl>());
        if (this->currentToken().m_kind == TokenKind::COMMA) {
            this->advance();
        } else if (this->currentToken().m_kind != TokenKind::CLOSE_PAR) {
            // TODO: throw
            throw UnexpectedToken({TokenKind::COMMA, TokenKind::CLOSE_PAR}, this->currentToken(), m_pos);
        }
    }

    // Ignore last comma, if any
    if (this->currentToken().m_kind == TokenKind::COMMA) {
        this->advance();
    }

    if (this->currentToken().m_kind == TokenKind::CLOSE_PAR) {
        this->advance();
    }

    // Now, parse the "optional" type of the function
    std::unique_ptr<Expr> retype = nullptr;
    if (this->currentToken().m_kind == TokenKind::COLON_COLON) {
        this->advance();
        this->parserIdentifier();
        retype = this->popAs<Identifier>();
    }

    // Now, parse the body
    std::unique_ptr<Block> body = nullptr;
    if (this->currentToken().m_kind == TokenKind::OPEN_CURLBRAC) {
        this->parseBlock();
        body = this->popAs<Block>();
    } else {
        // TODO: throw
        throw UnexpectedToken({TokenKind::OPEN_CURLBRAC}, this->currentToken(), m_pos);
    }

    this->push(std::make_unique<FctDecl>(name->name(), std::move(args), std::move(retype), std::move(body)), start);
}


void SimpleParser::parseLiteral() {
    const SourceLocation start = this->currentToken().m_location;
    // Let's see the first token in the stip do decide what to parse
    switch (this->currentToken().m_kind) {
        //  ---- Literals
        case TokenKind::INT_LIT: {
            std::int64_t value = std::stoll(this->currentToken().m_value);
            this->advance();
            this->push(std::make_unique<IntLiteral>(value), start);
            return;
        }
        case TokenKind::FLOAT_LIT: {
            double value = std::stod(this->currentToken().m_value);
            this->advance();
            this->push(std::make_unique<FloatLiteral>(value), start);
            return;
        }
        case TokenKind::BOOL_LIT: {
            bool value = (this->currentToken().m_value == "true");
            this->advance();
            this->push(std::make_unique<BoolLiteral>(value), start);
            return;
        }
        case TokenKind::STR_LIT: {
            // Strip the surrounding quotes the lexer kept.
            std::string value = this->currentToken().m_value;
            if (value.size() >= 2) {
                value = value.substr(1, value.size() - 2);
            }
            this->advance();
            this->push(std::make_unique<StringLiteral>(std::move(value)), start);
            return;
        }
    }
    return;
}

// Pushes exactly one Identifier -- nothing else. What follows the identifier is
// the caller's business, since only the caller knows the context it is in.
void SimpleParser::parserIdentifier() {
    if (this->currentToken().m_kind != TokenKind::IDENDIFIER) {
        throw UnexpectedToken({TokenKind::IDENDIFIER}, this->currentToken(), m_pos);
    }
    const Token& tok = this->advance();
    this->push(std::make_unique<Identifier>(tok.m_value), tok.m_location);
    return;
}

void SimpleParser::parseTranslationUnit() {
    const SourceLocation start = this->currentToken().m_location;
    auto unit = std::make_unique<TranslationUnit>();
    while (this->currentToken().m_kind != TokenKind::NONE) {
        this->parseStmt();
        unit->add(this->popAs<Stmt>());
    }
    this->push(std::move(unit), start);
    return;
}

void SimpleParser::buildAST() {
    this->parseTranslationUnit();
    this->m_root = this->popAs<TranslationUnit>();
    return;
}

} // namespace blast::core::parser