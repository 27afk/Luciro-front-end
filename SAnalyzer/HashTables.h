#pragma once
#include <vector>
#include <unordered_map>
#include <cstring> // Required for memcmp
#include "../Parser/AST.h"


// in case i forget what this does: this is the symbol table and scope management system for the semantic analyzer AKA id card for every variable and function :)
struct Symbol {
    StringView name;
    TokenType type;
    int stackOffset;
    bool isArray;
    int arraySize;
	int Structsize; // for struct types, to know total size
	TokenType BaseType; // for arrays, to know what type of elements it holds
	TokenType returnType; // for functions, to know what type it returns
    StringView StructType; // for structures
};

// Equality operator for StringView so the map can verify keys
inline bool operator==(const StringView& lhs, const StringView& rhs) {
    if (lhs.size != rhs.size) return false;
    return std::memcmp(lhs.data, rhs.data, lhs.size) == 0;
}

// Custom Hasher for StringView (FNV-1a Algorithm)
struct StringViewHasher {
    size_t operator()(const StringView& sv) const {
        size_t hash = 2166136261u;
        for (int i = 0; i < sv.size; ++i) {
            hash ^= static_cast<size_t>(sv.data[i]);
            hash *= 16777619u;
        }
        return hash;
    }
};

// vvvvvvv In case i forget: this is a single scope level in the symbol table stack vvvvvvvv
// Represents a single scope level AKA stuff between {} or global scope 
// (Global is 0 ex functoin declarations, just functoins code is 1, things like the code in if/while is 2+)
struct Scope {
    Scope* parent = nullptr;
    int level = 0;

    // We MUST pass the Hasher here so the map knows how to handle StringView
    std::unordered_map<StringView, Symbol, StringViewHasher> symbols;

    bool declare(const Symbol& sym) {
        // emplace returns a pair, .second is true if insertion was successful
        return symbols.emplace(sym.name, sym).second;
    }
	// looks for symbol in current scope only
    Symbol* lookupLocal(StringView name) {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &it->second;
    }
    // looks for symbol everywhere in scope chain
    Symbol* lookup(StringView name) {
        for (Scope* s = this; s; s = s->parent) {
            if (auto* sym = s->lookupLocal(name))
                return sym;
        }
        return nullptr;
    }
};

// vvvvv in case i forget vvvvv
// this is where all scopes are managed as a stack but no popping to preserve memory for later analysis
struct ScopeStack {
    std::vector<Scope*> allScopes;
    Scope* currentScope = nullptr;
	// add a scope on top of the stack
    void push() {
        // Create new scope linked to current
        Scope* newScope = new Scope();
        newScope->parent = currentScope;

        // Inherit level (Global is 0, first function is 1, etc.)
        if (currentScope) {
            newScope->level = currentScope->level + 1;
        }
        else {
            newScope->level = 0;
        }

        allScopes.push_back(newScope);
        currentScope = newScope;
    }
	// exit current scope and go back to parent
    void exit() {
        if (currentScope && currentScope->parent) {
            currentScope = currentScope->parent;
        }
        // Note: We don't exit the Global scope (parent == nullptr)
    }
	// look up symbol in current scope stack
    Symbol* lookup(StringView name) {
        return currentScope ? currentScope->lookup(name) : nullptr;
    }

    ~ScopeStack() {
        for (Scope* s : allScopes) {
            delete s;
        }
    }
};