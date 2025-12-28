#include <iostream>
#include <string>
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "SAnalyzer/SAnalyzer.h"
/*
Semantic Error at [13:13]: Base is not a struct.
Semantic Error at [13:17]: Type mismatch in assignment.
Semantic Error at [14:19]: Base is not a struct.
*/
int main() {
    // 1. The Luciro Source Code
    // code vec2 and player to test nested structs
    const char* source =
        "struct Vec2 {\n"
        "    int x;\n"
        "    int y;\n"
        "}\n"
        "\n"
        "struct Player {\n"
        "    Vec2 pos;\n"
        "    int health;\n"
        "}\n"
        "\n"
        "int bob(int x){ int y = x + 5; return x;}\n"
        "int main() {\n"
        "    int x;\n"
        "    bob(x);"
        "    Player p;\n"
        "    p.pos.x = 10;\n" // This tests the nested MemberAccess resolution
        "    return p.pos.x;\n"
        "}\n";

    std::cout << "--- Luciro Pipeline Test ---" << std::endl;
    std::cout << "Source Code:\n" << source << "\n\n";

    // 2. Lexical Analysis
    Lexer lexer(source);

    // 3. Parsing
    // The Parser constructor automatically tokenizes the whole stream
    Parser parser(lexer);
    ASTNode* root = parser.ParseProgram();

    if (!root) {
        std::cerr << "Parsing failed!" << std::endl;
        return 1;
    }
    std::cout << "[1/2] Parsing Successful.\n";

    // 4. Semantic Analysis
    // 'false' means BaJavMode is off (strict checking)
    SAnalyzer analyzer(false);

    try {
        std::cout << "[2/2] Starting Semantic Analysis...\n";
        root->accept(&analyzer);
        std::cout << "Analysis Complete: No Errors Found.\n";
    }
    catch (...) {
        std::cerr << "An unexpected error occurred during analysis." << std::endl;
        return 1;
    }

    std::cout << "\n--- Test Passed! ---" << std::endl;
    return 0;
}