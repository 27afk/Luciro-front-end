#include "Parser.h" // for Parser class
#include <vector> // for clothesline
#include <stack> // for shunting yard algorithm
#include <queue> // for shunting yard algorithm
#include <iostream> // for error output
#include <string> // for some shenanigans

/*
    HELPER FUNCTIONS
*/
// Swapped to template method for node creation to capture line/col and to manage memory and to shorten code
bool Parser::match(TokenType type) {
    if (currentToken.type == type) {
        advance();
        return true;
    }
    return false;
}

Token Parser::Peek(int n) {
    if (pos + n >= tokens.size()) return tokens.back();
    return tokens[pos + n];
}

void Parser::advance() {
    if (pos < tokens.size() - 1) {
        pos++;
        currentToken = tokens[pos];
    }
}

void Parser::error(const char* message) {
    std::cerr << "[Parser Error] Line " << currentToken.line << ", Column " << currentToken.column << ": " << message << std::endl;
    exit(1);
}

bool Parser::isType(TokenType type) {
    return type == TokenType::Integer ||
        type == TokenType::Double ||
        type == TokenType::Char ||
        type == TokenType::Bool ||
        type == TokenType::Struct ||
		type == TokenType::Identifier; // <-- user defined types
}

Token Parser::consume(TokenType Tok) {
    if (currentToken.type == Tok) {
        Token consumedToken = currentToken;
        advance();
        return consumedToken;
    }
    error("Unexpected token");

    return Token(TokenType::UNKNOWN, "", 0, -1, -1);
}

int Parser::getPrec(TokenType type) {
    if (type == TokenType::Not) return 5;
    if (type == TokenType::OpStar || type == TokenType::OpSlash) return 4;
    if (type == TokenType::OpPlus || type == TokenType::OpMinus) return 3;

    if (type == TokenType::OpLess || type == TokenType::OpGreater ||
        type == TokenType::OpIsLessEqual || type == TokenType::OpIsGreaterEqual) {
        return 2;
    }

    if (type == TokenType::OpIsEqual || type == TokenType::OpIsNotEqual) {
        return 1;
    }

    if (type == TokenType::OpAssign) return 0; // Assignment is lowest

    return -1;
}

bool Parser::isOperator(TokenType type) {
    return type == TokenType::OpPlus || type == TokenType::OpMinus ||
        type == TokenType::OpStar || type == TokenType::OpSlash ||
        type == TokenType::OpMod || type == TokenType::OpLess ||
        type == TokenType::OpGreater || type == TokenType::Not ||
        type == TokenType::OpIsLessEqual || type == TokenType::OpIsGreaterEqual ||
        type == TokenType::OpIsEqual || type == TokenType::OpIsNotEqual ||
        type == TokenType::OpAssign;

}

queue <Token> Parser::exprHelp(queue <Token> outputQueue, stack <Token> operatorStack) {
    while (operatorStack.size() > 0) {
        Token BufferToken = operatorStack.top();
        if (BufferToken.type == TokenType::LParen || BufferToken.type == TokenType::RParen) {
            error("Mismatched parentheses");
        }
        else {
            outputQueue.push(BufferToken);
            operatorStack.pop();
        }
    }
    return outputQueue;
}

/*
    EXPRESSION PARSING FUNCTIONS
*/

/*

b) {
        pos.x = a + b;
    }

    return pos.x;
}
*/
ExpressionNode* Parser::parsePrimary() {
    ExpressionNode* node = nullptr;
    // 1. Parse the "Base"
    if (match(TokenType::LParen)) {
        node = ExpressionParse();
        consume(TokenType::RParen);
    }
    else if (currentToken.type == TokenType::Integer || currentToken.type == TokenType::Double) {
        node = makeNode<LiteralNode>(currentToken.type, StringView{ currentToken.value, currentToken.Vsize });
        advance();
    }
    else if (currentToken.type == TokenType::Identifier) {
        node = makeNode<VariableExprNode>(StringView{ currentToken.value, currentToken.Vsize });
        advance();
    }
    // 2. The "Chaining" Loop (Crucial for pos.x)
    while (true) {
        if (match(TokenType::Dot)) {
            // After a '.', we MUST find an identifier (the member name)
            Token member = consume(TokenType::Identifier);
            node = makeNode<MemberAccessNode>(node, StringView{ member.value, member.Vsize });
        }
        else if (match(TokenType::LBrack)) {
            ExpressionNode* index = ExpressionParse();
            consume(TokenType::RBrack);
            node = makeNode<ArrayIndexNode>(node, index);
        }
        else if (match(TokenType::LParen)) {
            std::vector<ExpressionNode*> args;
            if (currentToken.type != TokenType::RParen) {
                do { args.push_back(ExpressionParse()); } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen);
            node = makeNode<FunctionCallNode>(node, args);
        }
        else {
            break; // No more dots or brackets, exit loop
        }
    }
    return node;
}
ExpressionNode* Parser::ExpressionParse() {
    std::stack<ExpressionNode*> nodeStack;
    std::stack<Token> operatorStack;

    auto processOperator = [&]() {
        if (operatorStack.empty()) return;
        Token op = operatorStack.top();
        operatorStack.pop();

        if (op.type == TokenType::Not) {
            if (nodeStack.empty()) { error("Expected operand for '!'"); return; }
            ExpressionNode* child = nodeStack.top(); nodeStack.pop();
            nodeStack.push(makeNode<UnaryOpNode>(child, op.type));
        }
        else {
            if (nodeStack.size() < 2) { error("Insufficient operands for operator"); return; }
            ExpressionNode* right = nodeStack.top(); nodeStack.pop();
            ExpressionNode* left = nodeStack.top(); nodeStack.pop();

            // FIXED: Only one check for OpAssign, and we only pop once for 'left' and 'right'
            if (op.type == TokenType::OpAssign) {
                nodeStack.push(static_cast<ExpressionNode*>(makeNode<AssignmentNode>(left, right)));
            }
            else {
                nodeStack.push(makeNode<BinaryOpNode>(op.type, left, right));
            }
        }
        };

    while (currentToken.type != TokenType::Semicolon &&
        currentToken.type != TokenType::RParen &&
        currentToken.type != TokenType::Comma &&
        currentToken.type != TokenType::Eof) {
        if (currentToken.type == TokenType::Identifier ||
            currentToken.type == TokenType::Integer ||
            currentToken.type == TokenType::Double ||
            currentToken.type == TokenType::Char ||
            currentToken.type == TokenType::LParen) {

            // parsePrimary correctly handles pos.x before returning here
            nodeStack.push(parsePrimary());
            continue;
        }
        else if (isOperator(currentToken.type)) {
            while (!operatorStack.empty() &&
                operatorStack.top().type != TokenType::LParen &&
                getPrec(operatorStack.top().type) >= getPrec(currentToken.type)) {
                processOperator();
            }
            operatorStack.push(currentToken);
            advance();
        }
        else {
            break;
        }
    }

    while (!operatorStack.empty()) {
        processOperator();
    }

    return nodeStack.empty() ? nullptr : nodeStack.top();
}

/*
        STATEMENT PARSERS
*/

StatementNode* Parser::ParseDeclaration() {
    Token typeToken = currentToken;
    advance();

    Token nameToken = consume(TokenType::Identifier);
    StringView nameView = { nameToken.value, (size_t)nameToken.Vsize };

    // Determine if this is a primitive or a custom struct
    StringView structTypeName = { nullptr, 0 };
    TokenType finalType = typeToken.type;

    if (typeToken.type == TokenType::Identifier) {
        finalType = TokenType::Struct;
        structTypeName = { typeToken.value, (size_t)typeToken.Vsize };
    }

    int size = -1;
    bool inferredSize = false;

    // Handle Array Brackets: int list[5]
    if (match(TokenType::LBrack)) {
        if (currentToken.type == TokenType::Integer) {
            size = std::stoi(std::string(currentToken.value, currentToken.Vsize));
            advance();
        }
        else if (currentToken.type == TokenType::RBrack) {
            inferredSize = true;
        }
        else {
            error("Expected array size or ']'");
        }
        consume(TokenType::RBrack);
    }

    ExpressionNode* singleInitializer = nullptr;
    std::vector<ExpressionNode*> arrayInitializers;

    // Handle Initialization: = ...
    if (match(TokenType::OpAssign)) {
        if (currentToken.type == TokenType::LBrace) {
            advance(); // consume '{'
            if (currentToken.type != TokenType::RBrace) {
                do {
                    arrayInitializers.push_back(ExpressionParse());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RBrace);

            if (inferredSize) {
                size = (int)arrayInitializers.size();
            }
        }
        else {
            // Single value initialization
            if (size != -1 || inferredSize) {
                arrayInitializers.push_back(ExpressionParse());
            }
            else {
                singleInitializer = ExpressionParse();
            }
        }
    }

    consume(TokenType::Semicolon);

    if (size != -1 || inferredSize) {
        return makeNode<ArrayDeclNode>(finalType, nameView, size, arrayInitializers);
    }
    else {
        // Pass structTypeName so SAnalyzer can look up the blueprint
        return makeNode<VarDeclNode>(finalType, nameView, structTypeName, singleInitializer);
    }
}

StatementNode* Parser::ParseBlock() {
    consume(TokenType::LBrace);
    std::vector<StatementNode*> blockStatements;

    while (currentToken.type != TokenType::RBrace && currentToken.type != TokenType::Eof) {
        StatementNode* stmt = ParseStatement();
        if (stmt) {
            blockStatements.push_back(stmt);
        }
        else {
            advance();
        }
    }

    if (currentToken.type == TokenType::RBrace) {
        consume(TokenType::RBrace);
    }
    else {
        error("Expected '}' at end of block");
    }

    return makeNode<BlockNode>(blockStatements);
}

StatementNode* Parser::ParseIfStatement() {
    consume(TokenType::If);
    consume(TokenType::LParen);
    ExpressionNode* condition = ExpressionParse();
    consume(TokenType::RParen);

    BlockNode* thenBranch = static_cast<BlockNode*>(ParseBlock());

    BlockNode* elseBranch = nullptr;
    if (currentToken.type == TokenType::Else) {
        advance();
        elseBranch = static_cast<BlockNode*>(ParseBlock());
    }

    return makeNode<IfStatementNode>(condition, thenBranch, elseBranch);
}

StatementNode* Parser::ParseWhileStatement() {
    consume(TokenType::While);
    consume(TokenType::LParen);
    ExpressionNode* condition = ExpressionParse();
    consume(TokenType::RParen);
    BlockNode* body = static_cast<BlockNode*>(ParseBlock());
    return makeNode<WhileStatementNode>(condition, body);
}

StatementNode* Parser::ParseReturnStatement() {
    consume(TokenType::Return);
    ExpressionNode* value = nullptr;
    if (currentToken.type != TokenType::Semicolon) {
        value = ExpressionParse();
    }
    consume(TokenType::Semicolon);
    return makeNode<ReturnStatementNode>(value);
}

StructDeclNode* Parser::ParseStructDeclaration() {
    consume(TokenType::Struct);
    Token nameToken = consume(TokenType::Identifier);
    StringView nameView = { nameToken.value, (size_t)nameToken.Vsize };

    consume(TokenType::LBrace);
    std::vector<StructMember> members;

    while (currentToken.type != TokenType::RBrace && currentToken.type != TokenType::Eof) {
        // If the first token is an Identifier, it's a nested struct (e.g., Vec2 pos;)
        if (currentToken.type == TokenType::Identifier) {
            Token typeToken = currentToken; // The type name "Vec2"
            advance();
            Token varToken = consume(TokenType::Identifier); // The variable name "pos"
            consume(TokenType::Semicolon);

            members.push_back({
                TokenType::Struct,
                { varToken.value, (size_t)varToken.Vsize },
                { typeToken.value, (size_t)typeToken.Vsize }
                });
        }
        // Otherwise, it's a primitive (e.g., int x;)
        else {
            TokenType pType = currentToken.type;
            advance();
            Token varToken = consume(TokenType::Identifier);
            consume(TokenType::Semicolon);

            members.push_back({
                pType,
                { varToken.value, (size_t)varToken.Vsize },
                { nullptr, 0 } // No struct type name for primitives
                });
        }
    }
    consume(TokenType::RBrace);
    match(TokenType::Semicolon);

    // This calls the constructor we just added to AST.h
    return makeNode<StructDeclNode>(nameView, members);
}

StatementNode* Parser::ParseFunctionDeclaration() {
    Token typeToken = currentToken;
    advance();

    Token nameToken = consume(TokenType::Identifier);
    consume(TokenType::LParen);

    std::vector<std::pair<TokenType, StringView>> params;
    if (currentToken.type != TokenType::RParen) {
        do {
            TokenType pType = currentToken.type;
            advance();
            Token pName = consume(TokenType::Identifier);
            params.emplace_back(pType, StringView{ pName.value, pName.Vsize });
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RParen);

    BlockNode* body = static_cast<BlockNode*>(ParseBlock());

    return makeNode<FunctionDeclNode>(StringView{ nameToken.value, nameToken.Vsize }, params, typeToken.type, body);
}

StatementNode* Parser::ParseStatement() {
    if (currentToken.type == TokenType::LBrace) return ParseBlock();
    if (currentToken.type == TokenType::If)     return ParseIfStatement();
    if (currentToken.type == TokenType::While)  return ParseWhileStatement();
    if (currentToken.type == TokenType::Return) return ParseReturnStatement();

    // NEW LOGIC:
    // A declaration is: (A primitive type) OR (An identifier followed by another identifier)
    bool isPrimitive = (currentToken.type == TokenType::Integer ||
        currentToken.type == TokenType::Double ||
        currentToken.type == TokenType::Char ||
        currentToken.type == TokenType::Bool);

    bool isUserTypeDecl = (currentToken.type == TokenType::Identifier && Peek(1).type == TokenType::Identifier);

    if (isPrimitive || isUserTypeDecl) {
        return ParseDeclaration();
    }

    // If it's not a declaration, it must be an expression (like pos.x = 10)
    ExpressionNode* expr = ExpressionParse();
    consume(TokenType::Semicolon);

    return makeNode<ExpressionStatementNode>(expr);
}

ASTNode* Parser::ParseProgram() {
    auto* program = makeNode<ProgramNode>();

    while (currentToken.type != TokenType::Eof) {

        if (currentToken.type == TokenType::Struct) {
            program->declarations.push_back(ParseStructDeclaration());
        }
        else if ((isType(currentToken.type) && Peek(2).type == TokenType::LParen) ||
            (currentToken.type == TokenType::Identifier && Peek(1).type == TokenType::LParen)) {

            program->declarations.push_back(ParseFunctionDeclaration());
        }
        else if (isType(currentToken.type)) {
            program->declarations.push_back(ParseDeclaration());
        }
        else {
            ExpressionNode* expr = ExpressionParse();
            if (expr) {
                consume(TokenType::Semicolon);
                program->declarations.push_back(makeNode<ExpressionStatementNode>(expr));
            }
            else {
                advance();
            }
        }
    }
    return program;
}