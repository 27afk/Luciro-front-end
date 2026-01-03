#pragma once

#include <vector>
#include "../SAnalyzer/HashTables.h"
#include "../SAnalyzer/Visitor.h"
#include "../Parser/AST.h"

enum class IROp {
    // Math
    ADD, SUB, MUL, DIV, MOD,

    // Bitwise (The "Hacker" Ops)
    AND, OR, XOR, SHL, SHR, // XOR, SHL, SHR will be hard coded 

    // Comparison
    LT, GT, LE, GE, EQ, NEQ, // <, >, <=, <=, ==, !=

    // Memory (The "Wonky" Core)
    LOAD, STORE, GET_ADDR, ASSIGN, LOAD_CONST, ALLOC,

    // Unary Operators
    NOT, NEG,

    // Flow
    LABEL, JUMP, IF_GOTO, IF_FALSE_GOTO,

    // Functions
    PARAM, CALL, RET,

    // Optimization 
    PHI, NOP
};


// string here for simplicity even tho not as efficient as const char* or StringView
struct Quad {
    IROp op;    // e.g., "+", "-", "ASSIGN", "CALL"
    int arg1;  // Left operand
    int arg2;  // Right operand
    int res;   // Where the result goes (the temporary)
};

class StringPool {
    std::vector<std::string> pool;
    std::unordered_map<std::string, int> lookup;
public:
    int getOrCreate(std::string name) {
        if (lookup.count(name)) return lookup[name];
        int id = pool.size();
        lookup[name] = id;
        pool.push_back(name);
        return id;
    }
    std::string getName(int id) { return pool[id]; }
};

class IRgen : public Visitor{
private:
    int labelCount = 0; 
    int tempCount = 0;  
    int lastResultId = -1; 
    const std::unordered_map<StringView, StructDeclNode*, StringViewHasher>* structRegistry;
public:
    StringPool Spool;
    std::vector <Quad> instructions;
    IRgen(const std::unordered_map<StringView, StructDeclNode*, StringViewHasher>& registry)
        : structRegistry(&registry) {
    }
    int nextTemp();
    int nextLabel();
    void emit(IROp op, int res, int arg1, int arg2);
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
    IROp opConvert(TokenType op);
    void Dump();
};