/*
*    BAJAV mode is the error suppression mode
*    where almost all or at least many errors are turned off
*    enabling faster compile times
*    and the freedom to do wonky things
*  
*    to use BAJAV mode simply type : "#BaJav#" at the start of ur program
*    **must be at the start**
*/
#include "Visitor.h"
#include <vector>
#include "../Parser/AST.h"
#include "SAnalyzer.h"
#include <iostream>
#include <string>

// implicit casting
bool SAnalyzer::isCompatible(TokenType target, TokenType source) {
    if (target == source) return true;
    if (target == TokenType::Integer && source == TokenType::Double) return true;
    if (target == TokenType::Double && source == TokenType::Integer) return true;
    return false;
}

// error reporting
void SAnalyzer::Error(int line, int col, const std::string& message) {
    if (BaJavMode) return;
    std::cerr << "Semantic Error at [" << line << ":" << col << "]: " << message << std::endl;
}

// basic visit function that goes through every node of program node
void SAnalyzer::visit(ProgramNode* node) {
    for (auto* decl : node->declarations) {
        if (decl) decl->accept(this);
    }
}

void SAnalyzer::visit(IfStatementNode* node) {
    if (node->condition) node->condition->accept(this);
    if (node->thenBranch) node->thenBranch->accept(this);
    if (node->elseBranch) node->elseBranch->accept(this);
}

void SAnalyzer::visit(BlockNode* node) {
    int prevOffset = this->nextOffset;
    scopeStack.push();
    for (auto* statement : node->statements) {
        statement->accept(this);
    }
    scopeStack.exit();
    this->nextOffset = prevOffset;
}

void SAnalyzer::visit(VarDeclNode* node) {
    if (node->initializer) node->initializer->accept(this);

    StringView typeNameString = { "", 0 };
    int size = 1;

    if (node->type == TokenType::Struct) {
        typeNameString = node->structTypeName; // "Player"
        Symbol* def = scopeStack.lookup(node->structTypeName);
        if (def) size = def->Structsize / 8;
    }

    Symbol sym = { node->name, node->type, nextOffset, false, 0, size * 8,
                   TokenType::UNKNOWN, TokenType::UNKNOWN, typeNameString };

    nextOffset += size;
    scopeStack.currentScope->declare(sym);
}
void SAnalyzer::visit(StructDeclNode* node) {
    int structTotalSize = 0;
    for (auto& member : node->members) {
        if (member.type == TokenType::Struct) {
            // Use the specific field for the type name
            Symbol* nestedDef = scopeStack.lookup(member.structTypeName);
            if (nestedDef) structTotalSize += nestedDef->Structsize;
            else structTotalSize += 8;
        }
        else {
            structTotalSize += 8;
        }
    }
    // put into struct registry 
    // i had a slight design error in parser so had to improvise a little
    structRegistry[node->name] = node; 
    Symbol sym = { node->name, TokenType::Struct, 0, false, 0, structTotalSize };
    scopeStack.currentScope->declare(sym);
}

void SAnalyzer::visit(AssignmentNode* node) {
    node->target->accept(this);
    node->value->accept(this);

    StringView targetName = node->target->getName();
    if (targetName.data != nullptr) {
        Symbol* sym = scopeStack.lookup(targetName);
        if (sym && BaJavMode) {
            sym->type = node->value->resolvedType;
        }
    }

    if (!BaJavMode && !isCompatible(node->target->resolvedType, node->value->resolvedType)) {
        Error(node->lin, node->col, "Type mismatch in assignment.");
    }
}

void SAnalyzer::visit(ArrayDeclNode* node) {
    int totalElements = node->initializers.size() > 0 ? node->initializers.size() : node->size;

    for (auto* init : node->initializers) if (init) init->accept(this);

    // Arrays take up 'totalElements' slots
    Symbol sym = { node->name, TokenType::List, nextOffset, true, totalElements, totalElements * 8, node->type, TokenType::UNKNOWN };
    nextOffset += totalElements;

    scopeStack.currentScope->declare(sym);
}

void SAnalyzer::visit(LiteralNode* node) {
    node->resolvedType = node->type;
}

void SAnalyzer::visit(BinaryOpNode* node) {
    node->left->accept(this);
    node->right->accept(this);

    if (node->left->resolvedType == TokenType::Double || node->right->resolvedType == TokenType::Double) {
        node->resolvedType = TokenType::Double;
    }
    else {
        node->resolvedType = TokenType::Integer;
    }

    if (!BaJavMode && !isCompatible(node->left->resolvedType, node->right->resolvedType)) {
        Error(node->lin, node->col, "Incompatible types in binary op.");
    }
}

void SAnalyzer::visit(VariableExprNode* node) {
    Symbol* sym = scopeStack.lookup(node->name);
    if (sym) {
        node->resolvedType = sym->type;
        // This 'seeds' the name "Player" into the node for the next dot to find
        if (sym->type == TokenType::Struct) {
            node->resolvedStructName = sym->StructType;
        }
    }
    else {
        node->resolvedType = TokenType::UNKNOWN;
        if (!BaJavMode) Error(node->lin, node->col, "Undefined variable.");
    }
}
void SAnalyzer::visit(ArrayIndexNode* node) {
    node->base->accept(this);
    node->index->accept(this);

    // We get the name of the base variable to check the table
    Symbol* sym = scopeStack.lookup(node->base->getName());
    if (sym && sym->isArray) {
        node->resolvedType = sym->BaseType; // result of list[i] is the BaseType
    }
    else {
        node->resolvedType = TokenType::UNKNOWN;
        if (!BaJavMode) Error(node->lin, node->col, "Base is not an array.");
    }

    if (!BaJavMode && node->index->resolvedType != TokenType::Integer) {
        Error(node->lin, node->col, "Array index must be an integer.");
    }
}

void SAnalyzer::visit(MemberAccessNode* node) {
    // 1. Visit the left side of dot
    node->structExpr->accept(this);

    // 2. Get the blueprint name (e.g., "Player")
    StringView typeToSearch = node->structExpr->resolvedStructName;

    // 3. Look up the blueprint in our registry
    if (structRegistry.count(typeToSearch)) {
        StructDeclNode* blueprint = structRegistry[typeToSearch];
        bool found = false;

        // 4. Search members
        for (const auto& member : blueprint->members) {
            // member.name is now a StringView in your StructMember struct
            if (member.name == node->memberName) {
                node->resolvedType = member.type;

                // 5. THE FIX: If it's a struct, pass the structTypeName 
                if (member.type == TokenType::Struct) {
                    node->resolvedStructName = member.structTypeName;
                }

                found = true;
                break;
            }
        }

        if (!found && !BaJavMode) {
            // Reconstruct string for error message
            std::string mName(node->memberName.data, node->memberName.size);
            std::string sName(typeToSearch.data, typeToSearch.size);
            Error(node->lin, node->col, "Member '" + mName + "' not found in struct '" + sName + "'");
        }
    }
    else {
        if (!BaJavMode) Error(node->lin, node->col, "Base is not a struct.");
        node->resolvedType = TokenType::UNKNOWN;
    }
}
void SAnalyzer::visit(FunctionDeclNode* node) {
    // Register function in the current scope (global)
    Symbol sym = { node->name, TokenType::Function, 0, false, 0, 0, TokenType::UNKNOWN, node->returnType };
    scopeStack.currentScope->declare(sym);

    // CREATE THE LOCAL SCOPE
    scopeStack.push();
    int savedOffset = this->nextOffset;
    this->nextOffset = 0; // Parameters start at offset 0 in the new frame
    StringView nameb = { nullptr,0};
    //STORE PARAMETERS IN THE LOCAL SCOPE
    for (auto& param : node->parameters) {
        Symbol paramSym = { param.second, param.first, nextOffset, false, 0 };
        scopeStack.currentScope->declare(paramSym);
        if (paramSym.type == TokenType::Struct) {

            nextOffset += paramSym.Structsize;
        }
        else {
            nextOffset += 8;
        }
    }

    if (node->body) {
        node->body->accept(this);
    }

    scopeStack.exit();
    this->nextOffset = savedOffset;
}
// skibditoilet(x,y);
void SAnalyzer::visit(FunctionCallNode* node) {
    if (node->callee) { node->callee->accept(this); }

    for (auto* arg : node->arguments) {
        if (arg) arg->accept(this);
    }

    StringView funcName = node->callee->getName();
    Symbol* sym = scopeStack.lookup(funcName);

    if (sym) {
        node->resolvedType = sym->returnType;
    }
    else {
        node->resolvedType = TokenType::Integer;

        if (!BaJavMode) {
            Error(node->lin, node->col, "Undefined function: " + std::string(funcName.data, funcName.size));
        }
    }
}
void SAnalyzer::visit(ReturnStatementNode* node) { 
    if (node->value) 
        node->value->accept(this); 
}
void SAnalyzer::visit(ExpressionStatementNode* node) { 
    if (node->expression) 
        node->expression->accept(this); 
}
void SAnalyzer::visit(WhileStatementNode* node) {
    if (node->condition) node->condition->accept(this);
    if (node->body) node->body->accept(this);
}

void SAnalyzer::visit(UnaryOpNode* node) {
    if (node->expression) {
        node->expression->accept(this);
        node->resolvedType = node->expression->resolvedType;
    }
}