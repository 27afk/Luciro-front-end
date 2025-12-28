#pragma once
#include <cstddef>

enum class TokenType {
    Identifier,
    // primitive data types
    Double,
    Integer,
    Char,
    Bool,
    List,
    // --- WORD-BASED OPERATORS ---
    OpAssign,        // 'assign' for =
    OpIsEqual,       // 'is' or 'equal' for ==
    OpIsNotEqual,    // 'is_not' for !=
    OpPlus,          // 'plus' for +
    OpMinus,         // 'minus' for -
    OpStar,          // 'times' for *
    OpSlash,         // 'div' for /
    OpMod,           // 'mod' for %
    OpLess,          // 'less' for <
    OpGreater,       // 'greater' for >
    OpIsLessEqual,   // 'less_equal' for <=
    OpIsGreaterEqual,// 'greater_equal' for >=

    // ADDED: LOGICAL OPERATORS
    OpOr,            // 'or' for logical OR
    OpAnd,           // 'and' for logical AND

    // key words and symbols (parentheses, braces are kept as symbols for clarity)
	LParen, RParen, LBrace, RBrace, LBrack, RBrack,
    Comma,           // ADDED: ',' for separating list/parameters
    Semicolon, Dot,     // ADDED: ';' for statement termination

    Def, Struct, Import, Return, If, Else, While, For,Function,
    Eof, UNKNOWN, Not, // !
    // FUN TIME KEYWORD
    BaJav // keyword to disable logic and grammar check in Semantic Analyzer >:)
};

struct Token {
    TokenType type;
    const char* value;
    int line;
    int column;
    size_t Vsize;
	Token() = default; // This brings back the "empty" struct ability
    Token(TokenType type, const char* lexeme, size_t size, int line, int column)
        : type(type), value(lexeme), Vsize(size), line(line), column(column) { }

};

