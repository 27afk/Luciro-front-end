#include <vector>
#include "IRgen.h"
#include "../SAnalyzer/HashTables.h"
#include <string>
#include "../SAnalyzer/Visitor.h"
#include "../Parser/AST.h"
#include <iostream>

// Generates a new unique temporary variable like "t4"
int IRgen::nextTemp() {
    std::string name = "t" + std::to_string(tempCount++);
    return Spool.getOrCreate(name);
}

// Generates a new unique jump target like "L2"
int IRgen::nextLabel() {
    std::string name = "L" + std::to_string(labelCount++);
    return Spool.getOrCreate(name);
}

// print everything out
void IRgen::Dump() {
    std::cout << "\n--- [Luciro IR Catalogue] ---\n";

    auto safeName = [&](int id) -> std::string {
        if (id == -1) return "___";
        return std::string(Spool.getName(id));
        };

    for (size_t i = 0; i < instructions.size(); ++i) {
        auto& q = instructions[i];
        std::cout << i << ": ";

        switch (q.op) {
            // Arithmetic
        case IROp::ADD: std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " + " << safeName(q.arg2); break;
        case IROp::SUB: std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " - " << safeName(q.arg2); break;
        case IROp::MUL: std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " * " << safeName(q.arg2); break;
        case IROp::DIV: std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " / " << safeName(q.arg2); break;
        case IROp::MOD: std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " % " << safeName(q.arg2); break;

            // Logical / Comparison
        case IROp::EQ:  std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " == " << safeName(q.arg2); break;
        case IROp::NEQ: std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " != " << safeName(q.arg2); break;
        case IROp::LT:  std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " < " << safeName(q.arg2); break;
        case IROp::GT:  std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " > " << safeName(q.arg2); break;
        case IROp::LE:  std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " <= " << safeName(q.arg2); break;
        case IROp::GE:  std::cout << safeName(q.res) << " = " << safeName(q.arg1) << " >= " << safeName(q.arg2); break;

            // Unary
        case IROp::NOT: std::cout << safeName(q.res) << " = NOT " << safeName(q.arg1); break;
        case IROp::NEG: std::cout << safeName(q.res) << " = NEG " << safeName(q.arg1); break;

            // Memory & Assignment
        case IROp::ASSIGN: std::cout << safeName(q.res) << " = " << safeName(q.arg2); break;
        case IROp::LOAD:   std::cout << safeName(q.res) << " = LOAD " << safeName(q.arg1); break;
        case IROp::STORE:  std::cout << "STORE " << safeName(q.res) << " <- " << safeName(q.arg1) << " (size: " << q.arg2 << ")"; break;
        case IROp::LOAD_CONST:
            std::cout << safeName(q.res) << " = CONST (" << safeName(q.arg1) << ")";
            break;
        case IROp::ALLOC: {
            int stride = (q.arg2 == -1) ? 8 : q.arg2; // Standard size is 8
            int count = (q.arg1 == -1) ? 1 : q.arg1;
            std::cout << safeName(q.res) << " ALLOC total_size=" << (count * stride)
                << " (" << count << " x " << stride << " bytes)";
            break;
        }

                        // Control Flow
        case IROp::LABEL:         std::cout << "LABEL " << safeName(q.res) << ":"; break;
        case IROp::JUMP:          std::cout << "JUMP " << safeName(q.res); break;
        case IROp::IF_FALSE_GOTO: std::cout << "IF NOT " << safeName(q.arg1) << " GOTO " << safeName(q.res); break;

            // Functions
        case IROp::PARAM: std::cout << "PARAM " << safeName(q.res); break;
        case IROp::CALL:  std::cout << safeName(q.res) << " = CALL " << safeName(q.arg1) << " (args: " << q.arg2 << ")"; break;
        case IROp::RET:   std::cout << "RET " << (q.arg1 != -1 ? safeName(q.arg1) : "void"); break;

        default: std::cout << "UNKNOWN OP"; break;
        }
        std::cout << "\n";
    }
    std::cout << "-----------------------------\n";
}
void IRgen::emit(IROp opp, int ress, int arg11, int arg22) {
    // 1. Package the information into a Quad
    Quad q;
    q.op = opp;     // What are we doing? (ADD, JUMP, etc.)
    q.res = ress;   // Where does the result go?
    q.arg1 = arg11; // First input
    q.arg2 = arg22; // Second input

    // 2. Add it to the final list
    instructions.push_back(q);
}

void Error(int line, int col, const std::string& message){
    std::cout << message << line << col << std::endl;
}

void IRgen::visit(ProgramNode* node)  {
    for (auto* decl : node->declarations) {
        if (decl) decl->accept(this);
    }
}
void IRgen::visit(BlockNode* node)  {
    for (auto* statement : node->statements) {
        if (statement) { statement->accept(this); }
    }
}

void IRgen::visit(StructDeclNode* node) {
 // blank to satisfy linker
}

// if statement
void IRgen::visit(IfStatementNode* node) {
    if (!node->condition) return;

    // 1. Evaluate the condition
    node->condition->accept(this);
    int condResult = this->lastResultId;

    // Create our "Bookmarker" Labels
    int elseLabel = nextLabel(); // Where the else starts
    int endLabel = nextLabel();  // Where the whole IF ends

    // The Fork: If condition is false, skip to else
    emit(IROp::IF_FALSE_GOTO, elseLabel, condResult, -1);

    //  The "Then" Branch
    if (node->thenBranch) {
        node->thenBranch->accept(this);
    }

    // The Escape: After 'then', jump to the very end
    emit(IROp::JUMP, endLabel, -1, -1);

    // The Else Branch
    emit(IROp::LABEL, elseLabel, -1, -1); // Mark the start of Else
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }
    
    emit(IROp::LABEL, endLabel, -1, -1); // Mark the end of the IF
}

IROp IRgen::opConvert(TokenType op) {
    switch (op) {
    case TokenType::OpPlus:
    { return IROp::ADD; }
    case TokenType::OpMinus:
    { return IROp::SUB; }
    case TokenType::OpStar:
    { return IROp::MUL; }
    case TokenType::OpSlash:
    { return IROp::DIV; }
    case TokenType::OpMod:
    { return IROp::MOD; }
    case TokenType::OpAssign:
    { return IROp::ASSIGN; }
    case TokenType::OpIsEqual:
    { return IROp::EQ; }
    case TokenType::OpIsNotEqual:
    { return IROp::NEQ; }
    case TokenType::OpLess:
    { return IROp::LT; }
    case TokenType::OpGreater:
    { return IROp::GT; }
    case TokenType::OpIsLessEqual:
    { return IROp::LE; }
    case TokenType::OpIsGreaterEqual:
    { return IROp::GE; }
    case TokenType::Not:
    { return IROp::NOT; }
    case TokenType::OpAnd:
    { return IROp::AND; }
    case TokenType::OpOr:
    { return IROp::OR; }
    }
}

/*
Label
Body
Jump
*/
/*
struct Quad {
    IROp op;    // e.g., "+", "-", "ASSIGN", "CALL"
    int arg1;  // Left operand
    int arg2;  // Right operand
    int res;   // Where the result goes (the temporary)
};*/
// 	std::vector <Quad> instructions;
// while statement
void IRgen::visit(WhileStatementNode* node) {
    if (!node->condition) return;

    // Create the location stamps
    int startLabel = nextLabel(); // Top of the loop
    int endLabel = nextLabel();   // Past the loop

    // Mark the Start
    emit(IROp::LABEL, startLabel, -1, -1);

    // Evaluate the condition
    node->condition->accept(this);
    int condResult = this->lastResultId;

    // The Exit: If condition is 0, leave the loop
    emit(IROp::IF_FALSE_GOTO, endLabel, condResult, -1);

    // Run the body
    if (node->body) {
        node->body->accept(this);
    }

    //  The Loop-back: Jump to the Start LABEL
    emit(IROp::JUMP, startLabel, -1, -1);

    // Mark the End
    emit(IROp::LABEL, endLabel, -1, -1);
}
// return statement
void IRgen::visit(ReturnStatementNode* node) {
    int returnValId = -1; // Default for "void" returns

    if (node->value) {
        // Calculate the math/expression first
        node->value->accept(this);
        // Grab the result from the "Clipboard"
        returnValId = this->lastResultId;
    }

    // Emit the RET instruction with the actual value ID
    emit(IROp::RET, -1, returnValId, -1);
}

// function
void IRgen::visit(FunctionDeclNode* node)  {
    std::string FName = { node->name.data, node->name.size };
    int funcID = Spool.getOrCreate(FName);
    emit(IROp::LABEL, funcID, -1, -1);
    // go thorugh params
    for (auto& param : node->parameters) {
        auto type = std::get<0>(param);
        auto name = std::get<1>(param);
        // param id
        int paramID = Spool.getOrCreate(std::string{ name.data, name.size });
        emit(IROp::PARAM, paramID, -1, -1);
    }
    if (node->body) {
        node->body->accept(this);
    }
    emit(IROp::RET, -1, -1, -1); // default to save user if they forgot on ein body
}

void IRgen::visit(VarDeclNode* node) {
    std::string varName = { node->name.data, node->name.size };
    int varID = Spool.getOrCreate(varName);

    int size = 8; // Default: not a struct (or standard 1-slot)

    if (node->type == TokenType::Struct) {
        auto it = structRegistry->find(node->structTypeName);
        if (it != structRegistry->end()) {
            size = it->second->totalSize;
        }
    }

    if (node->initializer) {
        node->initializer->accept(this);
        int initVal = this->lastResultId;
        // for mat emit ( IROp command, variable ID, size of variable, init value so -1 means none)
        // arg1 = value, arg2 = size metadata
        emit(IROp::ALLOC, varID, size, -1);
        emit(IROp::ASSIGN, varID, size, initVal);
    }
    else {
        // No value (arg1 = -1), but we still pass the size (arg2)
        emit(IROp::ALLOC, varID, size, -1);
    }
}

// variable assignment
void IRgen::visit(AssignmentNode* node) {
    // 1. Evaluate the Right-Hand Side (The Value)
    node->value->accept(this);
    int sourceValReg = this->lastResultId;

    node->target->accept(this);
    int destAddr = this->lastResultId;

    int size = 8;

    // Emit the store
    emit(IROp::STORE, destAddr, sourceValReg, size);

    // Pass value
    this->lastResultId = sourceValReg;
}
void IRgen::visit(ArrayDeclNode* node) {

    std::string arrayName = { node->name.data, node->name.size };
    int arrayID = Spool.getOrCreate(arrayName);

    // We need to pass the size to the backend.
    int numElements = node->size;
    int elementSize = 8; // Everything is 8 bytes in this wonky world
    if (node->type == TokenType::Struct) {
        auto it = structRegistry->find(node->structTypeName);
        elementSize = it->second->totalSize;
        std::cout << elementSize << std::endl << std::endl;
    }

    // Emit the ALLOC instruction
    emit(IROp::ALLOC, arrayID, numElements, elementSize);



    // If there are are initlizaers
    if (!node->initializers.empty()) {
        for (int i = 0; i < node->initializers.size(); ++i) {
            // Evaluate the initializer expression
            node->initializers[i]->accept(this);
            int valueReg = this->lastResultId;
            emit(IROp::STORE, arrayID, i, valueReg);
        }
    }
}
void IRgen::visit(ExpressionStatementNode* node)  {
    node->expression->accept(this);
    this->nextLabel();
}
void IRgen::visit(LiteralNode* node) {


    std::string valueStr = { node->getName().data, node->getName().size };


    // std::cout << "DEBUG: Literal value is '" << valueStr << "' length: " << node->getName().size << "\n";
    std::cout << "DEBUG: Literal Name Size: " << node->getName().size << std::endl;
    int valueID = Spool.getOrCreate(valueStr);
    int targetReg = nextTemp();

    emit(IROp::LOAD_CONST, targetReg, valueID, -1);
    this->lastResultId = targetReg;
}

// this->lastResultId = Spool.getOrCreate({ node->getName().data, node->getName().size });
void IRgen::visit(BinaryOpNode* node) {
    // 1. Handle Short-Circuiting Logical Operators
    if (node->op == TokenType::OpAnd || node->op == TokenType::OpOr) {
        int resultReg = nextTemp();
        int skipLabel = nextLabel();
        int endLabel = nextLabel();

        // Evaluate the Left side
        node->left->accept(this);
        int leftVal = this->lastResultId;

        if (node->op == TokenType::OpAnd) {
            // Logical AND: If Left is FALSE, jump to skipLabel (set to 0)
            emit(IROp::IF_FALSE_GOTO, skipLabel, leftVal, -1);

            // Evaluate the Right side
            node->right->accept(this);
            int rightVal = this->lastResultId;

            // Result is just whatever Right is (since Left was true)
            emit(IROp::ASSIGN, resultReg, -1, rightVal);
            emit(IROp::JUMP, endLabel, -1, -1);

            // Skip Path: Set result to 0
            emit(IROp::LABEL, skipLabel, -1, -1);
            int zeroID = Spool.getOrCreate("0");
            int zReg = nextTemp();
            emit(IROp::LOAD_CONST, zReg, zeroID, -1);
            emit(IROp::ASSIGN, resultReg, -1, zReg);
        }
        else { // Logical OR
            // If Left is TRUE, we skip Right. 
            // reuse IF_FALSE by jumping to evalRight if false, 
            // otherwise set result to 1 and jump to end.
            int evalRightLabel = nextLabel();
            emit(IROp::IF_FALSE_GOTO, evalRightLabel, leftVal, -1);

            // Left was True: Set result to 1 and jump to end
            int oneID = Spool.getOrCreate("1");
            int oReg = nextTemp();
            emit(IROp::LOAD_CONST, oReg, oneID, -1);
            emit(IROp::ASSIGN, resultReg, -1, oReg);
            emit(IROp::JUMP, endLabel, -1, -1);

            // Left was False: Eval right
            emit(IROp::LABEL, evalRightLabel, -1, -1);
            node->right->accept(this);
            emit(IROp::ASSIGN, resultReg, -1, this->lastResultId);
        }

        emit(IROp::LABEL, endLabel, -1, -1);
        this->lastResultId = resultReg;
        return;
    }

    // 2. Handle Standard Arithmetic/Comparison Operators
    node->left->accept(this);
    int leftReg = this->lastResultId;

    node->right->accept(this);
    int rightReg = this->lastResultId;

    int resultReg = nextTemp();
    IROp Lop = opConvert(node->op);
    emit(Lop, resultReg, leftReg, rightReg);

    this->lastResultId = resultReg;
}

void IRgen::visit(UnaryOpNode* node)  {
    node->expression->accept(this);
    int leftReg = this->lastResultId;
    int resultReg = nextTemp();
    if (opConvert(node->op) == IROp::NOT) {
        emit(IROp::NOT, resultReg, leftReg, -1);
    }
    else if (node->op == TokenType::OpMinus) {
        emit(IROp::NEG, resultReg, leftReg, -1);
    }
    // pass it up
    this->lastResultId = resultReg;
}

void IRgen::visit(VariableExprNode* node)  {
    std::string name = { node->getName().data, node->getName().size };
    int nameID = Spool.getOrCreate(name);
    int targetReg = nextTemp();
    emit(IROp::LOAD, targetReg, nameID, -1);
    this->lastResultId = targetReg;
}
void IRgen::visit(ArrayIndexNode* node) {
    // 1. Get the base address of the array (e.g., 'arr' in 'arr[i]')
    node->base->accept(this);
    int baseAddr = this->lastResultId;

    // 2. Get the index value (e.g., 'i')
    node->index->accept(this);
    int indexReg = this->lastResultId;

    // 3. Calculate the byte offset (Offset = index * 8)
    int offsetReg = nextTemp();
    
    int sizeID = Spool.getOrCreate("8"); // Assuming 8-byte slots

    if (node->resolvedStructName.data != nullptr) {
        auto size = structRegistry->find(node->resolvedStructName);
        StructDeclNode* blueprint = size->second;
        sizeID = Spool.getOrCreate(std::to_string(blueprint->totalSize));
    }

    int eightReg = nextTemp();
    emit(IROp::LOAD_CONST, eightReg, sizeID, -1);

    // Multiply index by 8
    emit(IROp::MUL, offsetReg, indexReg, eightReg);

    int finalAddr = nextTemp();
    emit(IROp::ADD, finalAddr, baseAddr, offsetReg);

    // Pass it up
    this->lastResultId = finalAddr;
}
void IRgen::visit(MemberAccessNode* node) {
    // base address
    node->structExpr->accept(this);
    int baseAddr = this->lastResultId;

    // 2. Look up the blueprint using the name SAnalyzer "stamped" on the expression
    auto it = structRegistry->find(node->structExpr->resolvedStructName);
    StructDeclNode* blueprint = it->second;

    int offset = 0;
    // Walk the members to find the target and sum the sizes of preceding members
    for (auto& member : blueprint->members) {
        if (member.name == node->memberName) {
            break;
        }

        // If this preceding member is a struct, we need its full size
        if (member.type == TokenType::Struct) {
            auto nested = structRegistry->find(member.structTypeName);
            offset += (nested != structRegistry->end()) ? nested->second->totalSize : 8;
        }
        else {
            offset += 8; // Simple types are always 8
        }
    }

    // 4. Resulting Address = Base + Offset
    int memberAddr = nextTemp();
    int offsetConstId = Spool.getOrCreate(std::to_string(offset));

    emit(IROp::ADD, memberAddr, baseAddr, offsetConstId);

    this->lastResultId = memberAddr;
}
void IRgen::visit(FunctionCallNode* node) {
    // Visit arguments first to get their values into registers
    std::vector<int> argRegisters;
    for (auto* arg : node->arguments) {
        arg->accept(this);
        argRegisters.push_back(this->lastResultId);
    }

    // Emit PARAM instructions for each argument
    for (int i = 0; i < argRegisters.size(); ++i) {
        // res: the value, arg1: the argument index (optional but helpful)
        emit(IROp::PARAM, argRegisters[i], i, -1);
    }

    // Get the function name (callee)
    std::string funcName = { node->callee->getName().data, node->callee->getName().size };
    int funcID = Spool.getOrCreate(funcName);

    //  Create a register for the return value
    int returnReg = nextTemp();

    //  Emit the CALL instruction
    emit(IROp::CALL, returnReg, funcID, (int)argRegisters.size());

    //  Pass the return value up the tree
    this->lastResultId = returnReg;
}