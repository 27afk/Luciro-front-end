#include "AST.h"
#include "../SAnalyzer/Visitor.h"



// --- Root Node ---
void ProgramNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

// --- Statement Nodes ---
void BlockNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void IfStatementNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void WhileStatementNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ReturnStatementNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void FunctionDeclNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void VarDeclNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void StructDeclNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void AssignmentNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayDeclNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ExpressionStatementNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

// --- Expression Nodes ---
void LiteralNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BinaryOpNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void UnaryOpNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void VariableExprNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayIndexNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void MemberAccessNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void FunctionCallNode::accept(Visitor* visitor) {
    visitor->visit(this);
}