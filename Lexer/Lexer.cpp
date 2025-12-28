#include "Lexer.h"
#include <unordered_map>
#include <string>
#include <cctype>
#include <cstring>
#include <iostream>

using namespace std;

// ==========================
// Core movement
// ==========================

void Lexer::advance() {
    if (*current == '\0') return;
    if (*current == '\n') {
        Tline++;
        Tcolumn = 1;
    }
    else {
        Tcolumn++;
    }
    current++;
}

char Lexer::peek() const {
    if (*current == '\0')
        return '\0';
    return *(current + 1);
}

// ==========================
// Literals
// ==========================

Token Lexer::IntLiteral() {
    const char* start = current;
    int startLine = Tline;
    int startColumn = Tcolumn;

    while (isdigit(*current)) {
        advance();
    }

    return Token(TokenType::Integer, start, current - start, startLine, startColumn);
}

Token Lexer::DecLiteral() {
    const char* start = current;
    int startLine = Tline;
    int startColumn = Tcolumn;

    while (isdigit(*current))
        advance();

    if (*current == '.' && isdigit(peek())) {
        advance(); // consume '.'
        while (isdigit(*current))
            advance();
    }

    return Token(TokenType::Double, start, current - start, startLine, startColumn);
}

Token Lexer::CharLiteral() {
    if (*current != '\'')
        error("CharLiteral called on non-quote character");

    int startLine = Tline;
    int startColumn = Tcolumn;

    advance(); // skip opening '

    if (*current == '\0')
        error("Unterminated char literal");

    const char* start = current;

    if (*current == '\\') {
        advance(); // skip '\'
        if (*current == '\0')
            error("Invalid escape sequence");
        advance(); // consume escaped char
    }
    else {
        advance(); // normal character
    }

    if (*current != '\'')
        error("Expected closing '");

    advance(); // skip closing '

    // Length is current - start, but note that the token value 
    // usually excludes the quotes depending on your Token implementation
    return Token(TokenType::Char, start, current - start - 1, startLine, startColumn);
}

// ==========================
// Punctuation
// ==========================

Token Lexer::Punctuation_literal() {
    static const std::unordered_map<char, TokenType> punct = {
        {'(', TokenType::LParen},
        {')', TokenType::RParen},
        {'{', TokenType::LBrace},
        {'}', TokenType::RBrace},
        {'[', TokenType::LBrack},
        {']', TokenType::RBrack},
        {',', TokenType::Comma},
        {';', TokenType::Semicolon},
        {'.', TokenType::Dot}
    };
    auto it = punct.find(*current);
    if (it == punct.end()) {
        // Handle unexpected characters to avoid infinite loops
        char err[] = { *current, '\0' };
        error("Unknown punctuation: ");
        advance();
        return Token(TokenType::UNKNOWN, current - 1, 1, Tline, Tcolumn);
    }
    const char* start = current;
    int line = Tline;
    int col = Tcolumn;

    advance();
    return Token(it->second, start, 1, line, col);
}

Token Lexer::operator_literal() {
    const char* start = current;
    int startLine = Tline;
    int startColumn = Tcolumn;

    char first = *current;
    advance();
    char second = *current; // Lookahead (Peek)

    TokenType type = TokenType::UNKNOWN;

    // Direct character dispatch
    switch (first) {
    case '+': type = TokenType::OpPlus; break;
    case '-': type = TokenType::OpMinus; break;
    case '*': type = TokenType::OpStar; break;
    case '/': type = TokenType::OpSlash; break;
    case '%': type = TokenType::OpMod; break;
    case '=':
        if (second == '=') { type = TokenType::OpIsEqual; advance(); }
        else { type = TokenType::OpAssign; }
        break;
    case '!':
        if (second == '=') { type = TokenType::OpIsNotEqual; advance(); }
        break;
    case '<':
        if (second == '=') { type = TokenType::OpIsLessEqual; advance(); }
        else { type = TokenType::OpLess; }
        break;
    case '>':
        if (second == '=') { type = TokenType::OpIsGreaterEqual; advance(); }
        else { type = TokenType::OpGreater; }
        break;
    case '&':
        if (second == '&') { type = TokenType::OpAnd; advance(); }
        break;
    case '|':
        if (second == '|') { type = TokenType::OpOr; advance(); }
        break;
    }

    if (type == TokenType::UNKNOWN) {
        error("Unknown operator");
    }

    return Token(type, start, (int)(current - start), startLine, startColumn);
}

Token Lexer::identifier_literal() {
    const char* start = current;
    int startLine = Tline;
    int startColumn = Tcolumn;

    // Consume all valid identifier characters
    while (isalnum(*current) || *current == '_') {
        advance();
    }

    std::string word(start, current);
    std::cout << "word='" << word << "' length=" << word.size() << std::endl;
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"def", TokenType::Def},
        {"struct", TokenType::Struct},
        {"import", TokenType::Import},
        {"return", TokenType::Return},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"for", TokenType::For},
        {"double", TokenType::Double},
        {"int", TokenType::Integer},
        {"char", TokenType::Char},
        {"bool", TokenType::Bool},
        {"array", TokenType::List},
        {"True", TokenType::Bool},
        {"False", TokenType::Bool},
    };
    TokenType type;
    std::cout << "word='";
    for (char c : word) std::cout << (int)c << " ";
    std::cout << "' len=" << word.size() << std::endl;
    auto it = keywords.find(word);
    if (it != keywords.end()) {
        type = it->second; // keyword matched
    }
    else {
        type = TokenType::Identifier; // default
    }
    return Token(type, start, current - start, startLine, startColumn);
}

void Lexer::BaJav_literal() {
    const char* start = current;
    int startLine = Tline;
    int startColumn = Tcolumn;
    // Consume all valid identifier characters
    while ((isalnum(*current) || *current == '_') && *current!= '#') {
        advance();
    }
    std::string word(start, current);
	advance(); // skip ending #
    cout << word;
    if (word == "BaJav") {
        firstToken = true;
        return;
    }
    else {
        error("Unknown keyword");
        return;
    }
}

// ==========================
// Main dispatcher
// ==========================

Token Lexer::getToken() {
    // Skip whitespace
	if (*current == '#' && Tline == 1) { // BaJav mode
		cout << "Entering BaJav mode!" << endl;
        advance(); // skip #
		cout << "Processing BaJav token..." << endl;
		BaJav_literal();
		Tline = 1;  // now that we processed the Bajav token, reset line and column
		Tcolumn = 1;
		cout << "BaJav mode set to " << (firstToken ? "true" : "false") << endl;
    }

    while (*current != '\0' && (*current == ' ' || *current == '\t' || *current == '\r' || *current == '\n')) {
        advance();
    }

    // End of file
    if (*current == '\0') {
        return Token(TokenType::Eof, current, 0, Tline, Tcolumn);
    }

    // Character literal
    if (*current == '\'') {
        return CharLiteral();
    }

    // Numbers
    if (isdigit(*current)) {
        const char* look = current;
        while (isdigit(*look)) look++;

        if (*look == '.') {
            return DecLiteral();
        }
        return IntLiteral();
    }

    // Identifiers / Keywords
    if (isalpha(*current) || *current == '_') {
        return identifier_literal();
    }
    //operators
    if (strchr("+-*/%=!<>|&", *current)) {
        return operator_literal();
	}
    return Punctuation_literal();
}