#pragma once
#include <cstdio>
#include "util.h"

enum TokenType
{
    TOKEN_KEYWORD,
    TOKEN_SYMBOL,
    TOKEN_INTEGERCONST,
    TOKEN_STRINGCONST,
    TOKEN_IDENTIFIER,

    TOKEN_EOF,
};

enum Keyword
{
    KEYWORD_CLASS,
    KEYWORD_CONSTRUCTOR,
    KEYWORD_FUNCTION,
    KEYWORD_METHOD,
    KEYWORD_FIELD,
    KEYWORD_STATIC,
    KEYWORD_VAR,
    KEYWORD_INT,
    KEYWORD_CHAR,
    KEYWORD_BOOLEAN,
    KEYWORD_VOID,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_NULL,
    KEYWORD_THIS,
    KEYWORD_LET,
    KEYWORD_DO,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_RETURN
};

struct Token
{
    enum TokenType type;
    enum Keyword keyword;
    char* text;
    int length;
    int value;

    int lineNumber;
    int column;

    Buffer toString();

    bool equals(const char* match);
    void print(FILE* outputFile);

    bool isKeyword();
    bool isKeyword(Keyword v);

    bool isSymbol();
    bool isSymbol(char v);
};

class JackTokenizer
{
    public:
        JackTokenizer(char* path);

        Token getToken();
        void writeToFile();

    private:
        char* mAt;
        char* mPath;
        int mLine;
        int mCol;
        char mCurrentSymbol;

        void eatAllWhitespace();
        void advance(int amount = 1);
};
