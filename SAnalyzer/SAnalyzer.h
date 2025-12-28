#pragma once
#include "../Parser/AST.h"
#include "HashTables.h"
#include "Visitor.h"

class SAnalyzer : public Visitor {
    std::unordered_map<StringView, StructDeclNode*, StringViewHasher> structRegistry;
	ScopeStack scopeStack; // to manage scopes and symbol tables
	bool BaJavMode = false; // to track if BaJav mode is on
	int nextOffset = 0; // to track stack offsets for variables
public:
    SAnalyzer(bool freedom) : BaJavMode(freedom) {
		scopeStack.push(); // Start with global scope
    }
    // Redeclaring the "Function of Doom" checklist
    void Error(int line, int col, const std::string& message);
    void visit(ProgramNode* node) override;
    void visit(BlockNode* node) override;
    void visit(IfStatementNode* node) override;
    void visit(WhileStatementNode* node) override;
    void visit(ReturnStatementNode* node) override;
    void visit(FunctionDeclNode* node) override;
    void visit(VarDeclNode* node) override;
    void visit(StructDeclNode* node) override;
    void visit(AssignmentNode* node) override;
    void visit(ArrayDeclNode* node) override;
    void visit(ExpressionStatementNode* node) override;
    void visit(LiteralNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(VariableExprNode* node) override;
    void visit(ArrayIndexNode* node) override;
    void visit(MemberAccessNode* node) override;
    void visit(FunctionCallNode* node) override;
    // helper functions
    bool isCompatible(TokenType target, TokenType source);
};