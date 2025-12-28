#pragma once

#include "../Parser/AST.h"
#include "HashTables.h"
#include <vector>
#include <iostream>
#include <string>

// forward declarations of all AST node types 
class ProgramNode;
class BlockNode;
class IfStatementNode;
class WhileStatementNode;
class ReturnStatementNode;
class FunctionDeclNode;
class VarDeclNode;
class StructDeclNode;
class AssignmentNode;
class LiteralNode;
class BinaryOpNode;
class UnaryOpNode;
class VariableExprNode;
class ArrayIndexNode;
class MemberAccessNode;
class FunctionCallNode;
class ArrayDeclNode;
class ExpressionStatementNode;

class Visitor {
public:
    virtual ~Visitor() = default;

    // The 18 Declarations your SAnalyzer MUST implement
    virtual void visit(ProgramNode* node) = 0;
    virtual void visit(BlockNode* node) = 0;
    virtual void visit(IfStatementNode* node) = 0;
    virtual void visit(WhileStatementNode* node) = 0;
    virtual void visit(ReturnStatementNode* node) = 0;
    virtual void visit(FunctionDeclNode* node) = 0;
    virtual void visit(VarDeclNode* node) = 0;
    virtual void visit(StructDeclNode* node) = 0;
    virtual void visit(AssignmentNode* node) = 0;
    virtual void visit(ArrayDeclNode* node) = 0;
    virtual void visit(ExpressionStatementNode* node) = 0;
    virtual void visit(LiteralNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    virtual void visit(UnaryOpNode* node) = 0;
    virtual void visit(VariableExprNode* node) = 0;
    virtual void visit(ArrayIndexNode* node) = 0;
    virtual void visit(MemberAccessNode* node) = 0;
    virtual void visit(FunctionCallNode* node) = 0;
};