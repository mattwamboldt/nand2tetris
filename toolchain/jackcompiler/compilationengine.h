#pragma once
#include "jacktokenizer.h"
#include "symboltable.h"
#include "vmwriter.h"

class CompilationEngine
{
    public:
        /// <summary>
        /// Creates a new compilation engine with the given input and output. The next routine called must be compileClass
        /// </summary>
        CompilationEngine(char* inputPath, char* outputPath);

        /// <summary>
        /// Compiles a complete class.
        /// </summary>
        void compileClass();

        /// <summary>
        /// Compiles a static declaration or a field declaration.
        /// </summary>
        void compileClassVarDec();

        /// <summary>
        /// Compiles a complete method, function, or constructor.
        /// </summary>
        void compileSubroutine();

        /// <summary>
        /// Compiles a (possibly empty) parameter list, not including the enclosing "()".
        /// </summary>
        void compileParameterList();

        /// <summary>
        /// Compiles a var declaration.
        /// </summary>
        void compileVarDec();

        /// <summary>
        /// Compiles a sequence of statements, not including the enclosing "{}".
        /// </summary>
        void compileStatements();

        /// <summary>
        /// Compiles a do statement
        /// </summary>
        void compileDo();

        void compileSubroutineCall(Buffer subRoutineName);

        /// <summary>
        /// Compiles a let statement
        /// </summary>
        void compileLet();

        /// <summary>
        /// Compiles a while statement
        /// </summary>
        void compileWhile();

        /// <summary>
        /// Compile s a return statement
        /// </summary>
        void compileReturn();

        /// <summary>
        /// Compiles an if statement, possibly with a trailing else clause.
        /// </summary>
        void compileIf();

        /// <summary>
        /// Compiles an expression.
        /// </summary>
        void compileExpression();

        /// <summary>
        /// Compiles a term. This routine requires one token of lookahead to distinguish variables, array access, and subroutine calls.
        /// </summary>
        void compileTerm();

        /// <summary>
        /// Compiles a (possibly empty) comma-separated list of expressions
        /// </summary>
        int compileExpressionList();

    private:
        JackTokenizer mTokenizer;
        SymbolTable mSymbolTable;
        VMWriter mVMWriter;
        Token mCurrentToken;
        Buffer mClassName;
        char* mInputPath;
        bool mIsVoid;

        /// <summary>
        /// Read the next token and verify it as the given keyword
        /// </summary>
        /// <param name="expectedKeyword">The expected keyword</param>
        void readKeyword(Keyword expectedKeyword);

        /// <summary>
        /// Read the next token and verify it as a symbol. Optionally verify it is what we expect.
        /// </summary>
        /// <param name="expectedSymbol">(Optional) The expected symbol</param>
        void readSymbol(char expectedSymbol = 0);

        /// <summary>
        /// Read the next token and verify it has the given type
        /// </summary>
        /// <param name="expectedTokenType">The expected token type</param>
        void readToken(enum TokenType expectedTokenType);

        void unexpectedToken();

        void verifySymbol(char expectedSymbol);

        bool isOperator();
        bool isKeywordConstant();
};