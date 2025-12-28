#pragma once
/*
	I have a lot of pseudo code so i dont forget what my code is for since htis project is taking a while
*/
// logic for visit function ( so i dont forget ): SAnalyzer calls big visit function, which calls accept on node, which calls visit on specific node type
#include "../Lexer/Token.h"
#include <iostream>
#include <vector>

class ExpressionNode;
class StatementNode;
class Visitor; // foward declaration so circular depdency doesnt fry my brain

struct StringView {
	const char* data; // pointer to store data
	size_t size; // how many characters in data
};


struct StructMember { // for structs
	TokenType type;
	StringView name;
	StringView structTypeName; 
};


struct Parameter {
	TokenType type; // could be primitive type or array or even struct type (user defined) :))
	StringView name; // name of parameter 
};


// Base class for all AST nodes
class ASTNode {
	public:
	virtual ~ASTNode() = default;
	virtual void accept(Visitor* visitor) = 0; // so doesn't default to nothing
	int lin = 0;   // line number in source code
	int col = 0;
};
// Variable expressions :)
class ExpressionNode : public ASTNode {
public:
	StringView resolvedStructName = { "", 0 }; // Add this!
	TokenType resolvedType = TokenType::UNKNOWN;
	// get name if applicable (like VariableExprNode), else empty
	virtual StringView getName() { return { nullptr, 0 }; }
};
// Base class for all
class ProgramNode : public ASTNode {
public:
	// A list of everything at the top level: Structs, Functions, Globals
	std::vector<ASTNode*> declarations;

	ProgramNode() = default;
	void accept(Visitor* visitor);
};

// statement node and its children :)
class StatementNode : public ASTNode {
public:
	virtual void accept(Visitor* visitor) = 0; // pure virtual
};

class BlockNode : public StatementNode { // block of statements
public:
	std::vector <StatementNode*> statements;
	BlockNode(std::vector<StatementNode*> stmts)
		: statements(stmts) {
	}
	void accept(Visitor* visitor);
};

class IfStatementNode : public StatementNode { // if-else statement
public:
	ExpressionNode* condition;
	BlockNode* thenBranch;
	BlockNode* elseBranch;
	IfStatementNode(ExpressionNode* cond, BlockNode* thenB, BlockNode* elseB)
		: condition(cond), thenBranch(thenB), elseBranch(elseB) {
	}
	void accept(Visitor* visitor);
};

class WhileStatementNode : public StatementNode { // while loop
public:
	ExpressionNode* condition;
	BlockNode* body;
	WhileStatementNode(ExpressionNode* cond, BlockNode* b)
		: condition(cond), body(b) {
	}
	void accept(Visitor* visitor);

};

class ReturnStatementNode : public StatementNode { // return statement
public:
	ExpressionNode* value;
	ReturnStatementNode(ExpressionNode* val)
		:  value(val) {
	}
	void accept(Visitor* visitor);
};

class FunctionDeclNode : public StatementNode { // function declaration
public:
	StringView name; // name of function
	std::vector <std::pair<TokenType,StringView>> parameters; // parameters
	TokenType returnType; // return type/ type of function if u want a void funciton just dont make it equal anything  like int x(); just dont make it equal anything when u call it
	BlockNode* body; // function body
	FunctionDeclNode(StringView n, std::vector<std::pair<TokenType, StringView>> params, TokenType retType, BlockNode* b)
		: name(n), parameters(params), returnType(retType), body(b) {
	}
	void accept(Visitor* visitor);
};

class VarDeclNode : public StatementNode { // variable declaration
public:
	TokenType type;
	StringView name;
	StringView structTypeName; // for struct types, to know which struct it is
	ExpressionNode* initializer;
	VarDeclNode(TokenType t, StringView n, ExpressionNode* init)
		: type(t), name(n), initializer(init) {
	}
	VarDeclNode(TokenType t, StringView n, StringView stname, ExpressionNode* init)
		: type(t), name(n), structTypeName(stname), initializer(init) {
	}
	void accept(Visitor* visitor);
};

class StructDeclNode : public StatementNode {
public:
	StringView name;
	std::vector<StructMember> members;

	StructDeclNode(StringView n, std::vector<StructMember> m)
		: name(n), members(m) {
	}

	void accept(Visitor* visitor);
};

class AssignmentNode : public ExpressionNode { // variable assignment
public:
	ExpressionNode* target;
	ExpressionNode* value;
	AssignmentNode(ExpressionNode* target, ExpressionNode* value)
		: target(target), value(value) {}
	void accept(Visitor* visitor);
};

class LiteralNode : public ExpressionNode { // holds raw numbers, bools, chars
public:
	TokenType type;
	StringView value;
	LiteralNode(TokenType t, StringView val) : type(t), value(val) {}
	void accept(Visitor* visitor);
};

class BinaryOpNode : public ExpressionNode { // +, -, *, /, etc.
public:
	TokenType op; // operator type
	ExpressionNode* left; // note: left and right operands MUST be uploaded to clothesline so memoryleak doesn't explode my computer
	ExpressionNode* right;
	BinaryOpNode(TokenType oper, ExpressionNode* lhs, ExpressionNode* rhs)
		: op(oper), left(lhs), right(rhs) {
	}
	void accept(Visitor* visitor);
};

class UnaryOpNode : public ExpressionNode { // !, - (negation)
public:
	TokenType op;              // To store TokenType::OpNot (!)
	ExpressionNode* expression;  // expression gets unary op applied to
	UnaryOpNode(ExpressionNode* expr, TokenType opp) : expression(expr), op(opp) {}
	void accept(Visitor* visitor);
};

class VariableExprNode : public ExpressionNode { // get value of variable named name
public:
	// The name of the variable being referenced (e.g., "truck" or "x")
	StringView name;
	VariableExprNode(StringView n) : name(n) {}
	void accept(Visitor* visitor);
	StringView getName() override { return name; }
};
class ArrayIndexNode : public ExpressionNode {
public:
	ExpressionNode* base;  // This could be a Variable, another Array, or a Function Call
	ExpressionNode* index; // The value inside the [ ]

	ArrayIndexNode(ExpressionNode* b, ExpressionNode* i) : base(b), index(i) {}
	void accept(Visitor* visitor);
};

class MemberAccessNode : public ExpressionNode { // struct member access
public:
	StringView memberName;
	ExpressionNode* structExpr;
	MemberAccessNode(ExpressionNode* stpr, StringView mname)
		: structExpr(stpr), memberName(mname) {
	}
	void accept(Visitor* visitor);
};

class FunctionCallNode : public ExpressionNode {
public:
	ExpressionNode* callee; // Usually a VariableExprNode for the function name
	std::vector<ExpressionNode*> arguments;

	FunctionCallNode(ExpressionNode* c, std::vector<ExpressionNode*> a)
		: callee(c), arguments(a) {
	}
	void accept(Visitor* visitor);
};
// two more staement nodes
class ArrayDeclNode : public StatementNode {
public:
	TokenType type;      // int, double, etc.
	StringView name;     // the identifier
	int size;            // the fixed size (or an ExpressionNode* if dynamic)
	std::vector<ExpressionNode*> initializers; // optional initial values
	ArrayDeclNode(TokenType t, StringView n, int s, std::vector<ExpressionNode*> init)
		: type(t), name(n), size(s) ,initializers(std::move(init)) { // apparenlty std::move is more efficeint so why not
	}
	ArrayDeclNode(TokenType t, StringView n, int s) // without initializers
		: type(t), name(n), size(s) {
	}
	void accept(Visitor* visitor);
};

class ExpressionStatementNode : public StatementNode {
public:
	ExpressionNode* expression;
	ExpressionStatementNode(ExpressionNode* expr) : expression(expr) {}
	void accept(Visitor* visitor);
};