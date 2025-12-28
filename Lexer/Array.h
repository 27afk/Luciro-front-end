#pragma once

#include "Token.h"
#include <cstddef>
#include <iostream>
#include <vector>

struct UniversalArray {
    TokenType type;
    const char* name = nullptr;           // optional token name
    std::vector<const char*> elements;    // points into source buffer
    UniversalArray(TokenType t) : type(t) {}
    // helper to get number of elements
    size_t size() const {
        return elements.size();
    }
    // add element
    void addElement(const char* elem) {
        elements.push_back(elem);
    }
};
