#pragma once

#include "Token.h"
#include <iostream>

using namespace std;

class Lexer {
private:
	const char* source;
	int Tline = 1;
	int Tcolumn = 1;
public:
	bool firstToken = false;  // to track which mode the compiler is in
	Lexer(const char* src) : source(src), current(&source[0]) {}
	void advance();
	char peek()const;
	// bool isSpace();
	// void skipWhitespace();
	void error(const char* message) {
		cerr << "[Lexer Error] Line " << Tline << ", Column " << Tcolumn << ": " << message << endl;
		exit(1);
	}
	const char* current;
	Token operator_literal();
	Token IntLiteral();
	Token Punctuation_literal();
	Token DecLiteral();
	Token CharLiteral();;
	Token identifier_literal();
	void BaJav_literal();
	Token getToken();
};