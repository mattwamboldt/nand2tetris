#pragma once
#include "util.h"

enum SymbolKind
{
    SYMBOL_NONE,
    SYMBOL_STATIC,
    SYMBOL_FIELD,
    SYMBOL_ARG,
    SYMBOL_VAR,
    SYMBOL_COUNT
};

struct Symbol
{
    Buffer name = {};
    Buffer type = {};
    SymbolKind kind;
    int index;
};

class SymbolTable
{
    public:
        SymbolTable();

        /// <summary>
        /// Starts a new subroutine scope (i.e., resets the subroutine's symbol table).
        /// </summary>
        void startSubroutine();

        int define(Buffer name, Buffer type, SymbolKind kind);
        int varCount(SymbolKind kind);

        SymbolKind kindOf(Buffer name);
        Buffer typeOf(Buffer name);
        int indexOf(Buffer name);

        Symbol find(Buffer name);

    private:
        Symbol mClassTable[300];
        int mClassTableCount;

        Symbol mSubroutineTable[300];
        int mSubroutineTableCount;

        int mVarCounts[SYMBOL_COUNT];
};