#pragma once
#include "../Lexer/Lexer.h"
#include "../Lexer/Token.h"
#include "AST.h"
#include <vector>
#include <iostream>
#include <queue>
#include <stack>

// In Parser.h (private)


class Parser {
	private:
		/*
		Variables
		*/
	Lexer& lexer;
	Token currentToken;
	std::vector<Token> tokens;
	bool BaJavMode;
	int pos = 0;
	template <typename T, typename... Args>
	T* makeNode(Args&&... args) {
		T* node = new T(std::forward<Args>(args)...);
		clothesline.push_back(node);
		// Grab line/col from the token currently being processed
		node->lin = currentToken.line;
		node->col = currentToken.column;
		return node;
	}
	/*
	Expression Parsing
	*/
	std::vector<ASTNode*> clothesline; // to hold allocated nodes for cleanup
	ExpressionNode* ExpressionParse(); // shunting yard algorithm to parse expressions
	StatementNode* AssignmentParse(); // parse assignments also calls on ExpressionParse
	StatementNode* ParseDeclaration(); // parse variable declarations 
	Token consume(TokenType Tok); // consume expected token or error
	ExpressionNode* parsePrimary(); // parse primary expressions (identifiers, literals)

	/*
	Statements
	*/
	StatementNode* ParseBlock();
	StatementNode* ParseIfStatement();
	StatementNode* ParseWhileStatement();
	StatementNode* ParseFunctionDeclaration();
	StatementNode* ParseReturnStatement();
	StructDeclNode* ParseStructDeclaration();
	StatementNode* ParseStatement(); // general statement parser
	/*
	Helper functions and destructors and error handling function
	*/
	bool isType(TokenType type);
	int getPrec(TokenType type); // get operator precedence
	queue <Token> exprHelp(queue <Token> tokenQueue, stack <Token> operatorStack); // helps make shunting yard more readable
	bool isOperator(TokenType type); // check if token is operator
	void advance();
	Token Peek(int n);
	bool Check(TokenType Tok) { return currentToken.type == Tok; }// check current token type
	bool match(TokenType type); // match and advance if token matches
	void error(const char* message); // error handling
	/*
	Parser Constructor
	*/
public:
	Parser(Lexer& l) : lexer(l), pos(0) { 
		Token t = lexer.getToken(); 
		this->BaJavMode = lexer.firstToken;

		while (t.type != TokenType::Eof) {
			tokens.push_back(t);
			t = lexer.getToken();
		}
		tokens.push_back(t);

		if (!tokens.empty()) {
			currentToken = tokens[0];
		}
	}
	ASTNode* ParseProgram();
	// destructor to clean up clothesline	
	~Parser() {
		// destructor to clean up clothesline
		for (ASTNode* node : clothesline) {
			delete node;
		}
		clothesline.clear();
	}
};
