#include "symboltable.h"

SymbolTable::SymbolTable()
    :mClassTableCount(0), mSubroutineTableCount(0)
{
    mVarCounts[SYMBOL_STATIC] = 0;
    mVarCounts[SYMBOL_FIELD] = 0;
    mVarCounts[SYMBOL_ARG] = 0;
    mVarCounts[SYMBOL_VAR] = 0;
}

void SymbolTable::startSubroutine()
{
    mSubroutineTableCount = 0;
    mVarCounts[SYMBOL_ARG] = 0;
    mVarCounts[SYMBOL_VAR] = 0;
}

int SymbolTable::define(Buffer name, Buffer type, SymbolKind kind)
{
    int index = 0;
    if (kind == SYMBOL_STATIC || kind == SYMBOL_FIELD)
    {
        index = mVarCounts[kind]++;
        mClassTable[mClassTableCount].name = name;
        mClassTable[mClassTableCount].type = type;
        mClassTable[mClassTableCount].kind = kind;
        mClassTable[mClassTableCount].index = index;
        ++mClassTableCount;
    }
    else
    {
        index = mVarCounts[kind]++;
        mSubroutineTable[mSubroutineTableCount].name = name;
        mSubroutineTable[mSubroutineTableCount].type = type;
        mSubroutineTable[mSubroutineTableCount].kind = kind;
        mSubroutineTable[mSubroutineTableCount].index = index;
        ++mSubroutineTableCount;
    }

    return index;
}

int SymbolTable::varCount(SymbolKind kind)
{
    return mVarCounts[kind];
}

SymbolKind SymbolTable::kindOf(Buffer name)
{
    return find(name).kind;
}

Buffer SymbolTable::typeOf(Buffer name)
{
    return find(name).type;
}

int SymbolTable::indexOf(Buffer name)
{
    return find(name).index;
}

Symbol SymbolTable::find(Buffer name)
{
    for (int i = 0; i < mSubroutineTableCount; ++i)
    {
        if (mSubroutineTable[i].name.equals(name))
        {
            return mSubroutineTable[i];
        }
    }

    for (int i = 0; i < mClassTableCount; ++i)
    {
        if (mClassTable[i].name.equals(name))
        {
            return mClassTable[i];
        }
    }

    return Symbol();
}