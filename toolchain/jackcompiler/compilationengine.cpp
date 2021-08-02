#include <stdlib.h>
#include "compilationengine.h"

CompilationEngine::CompilationEngine(char* inputPath, char* outputPath)
    :mTokenizer(inputPath), mVMWriter(outputPath), mCurrentToken(),
    mInputPath(inputPath), mClassName(), mIsMethod(false), mConstructor(false),
    mWhileCount(0), mIfCount(0)
{
    
}

void CompilationEngine::compileClass()
{
    // 'class' className '{' classVarDec* subRoutineDec* '}'
    readKeyword(Keyword::KEYWORD_CLASS);

    readToken(TokenType::TOKEN_IDENTIFIER);
    mClassName = mCurrentToken.toString();

    readSymbol('{');

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isKeyword())
    {
        switch (mCurrentToken.keyword)
        {
            case KEYWORD_STATIC:
            case KEYWORD_FIELD:
                compileClassVarDec();
                break;

            case KEYWORD_CONSTRUCTOR:
            case KEYWORD_FUNCTION:
            case KEYWORD_METHOD:
                compileSubroutine();
                break;

            default:
                unexpectedToken();
        }

        mCurrentToken = mTokenizer.getToken();
    }

    verifySymbol('}');
}

void CompilationEngine::compileClassVarDec()
{
    // ('static' | 'field') type varName (',' varName)* ';'
    SymbolKind kind;
    if (mCurrentToken.keyword == KEYWORD_STATIC)
    {
        kind = SYMBOL_STATIC;
    }
    else
    {
        kind = SYMBOL_FIELD;
    }

    mCurrentToken = mTokenizer.getToken();
    Buffer type = mCurrentToken.toString();

    mCurrentToken = mTokenizer.getToken();
    mSymbolTable.define(mCurrentToken.toString(), type, kind);

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isSymbol(','))
    {
        mCurrentToken = mTokenizer.getToken();
        mSymbolTable.define(mCurrentToken.toString(), type, kind);

        mCurrentToken = mTokenizer.getToken();
    }
}

void CompilationEngine::compileSubroutine()
{
    // subroutineDec: ('constructor' | 'function' | 'method') ('void' | type) subRoutineName '(' parameterList ')' subroutineBody
    mSymbolTable.startSubroutine();
    mWhileCount = 0;
    mIfCount = 0;

    Keyword subroutineType = mCurrentToken.keyword;
    mCurrentToken = mTokenizer.getToken();

    // return type
    bool isVoid = mCurrentToken.isKeyword(KEYWORD_VOID);
    Buffer type = mCurrentToken.toString();
    mCurrentToken = mTokenizer.getToken();

    // subroutineName
    Buffer subroutineName = mCurrentToken.toString();

    readSymbol('(');
    compileParameterList();
    verifySymbol(')');

    // subroutineBody: '{' varDec* statements '}'
    readSymbol('{');

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isKeyword(KEYWORD_VAR))
    {
        compileVarDec();
        mCurrentToken = mTokenizer.getToken();
    }

    mVMWriter.writeFunction(mClassName, subroutineName, mSymbolTable.varCount(SYMBOL_VAR));
    if (subroutineType == KEYWORD_METHOD)
    {
        // set our this pointer to arg 0
        // pointer 0 = this, pointer 1 = THAT
        mVMWriter.writePush(VMSegment::SEGMENT_ARG, 0);
        mVMWriter.writePop(VMSegment::SEGMENT_POINTER, 0);
        mIsMethod = true;
    }
    else if (subroutineType == KEYWORD_CONSTRUCTOR)
    {
        mVMWriter.writePush(SEGMENT_CONST, mSymbolTable.varCount(SYMBOL_FIELD));
        mVMWriter.writeCall("Memory.alloc", 1);
        mVMWriter.writePop(VMSegment::SEGMENT_POINTER, 0);
        mConstructor = true;
    }

    compileStatements();
    mIsMethod = false;
    mConstructor = false;

    verifySymbol('}');
}

void CompilationEngine::compileParameterList()
{
    // ((type varName) (',' type varName)*)?
    mCurrentToken = mTokenizer.getToken();
    if (mCurrentToken.type == TOKEN_IDENTIFIER || mCurrentToken.isKeyword())
    {
        Buffer type = mCurrentToken.toString();

        mCurrentToken = mTokenizer.getToken();
        mSymbolTable.define(mCurrentToken.toString(), type, SYMBOL_ARG);
        
        mCurrentToken = mTokenizer.getToken();
    }

    while (mCurrentToken.isSymbol(','))
    {
        // type
        mCurrentToken = mTokenizer.getToken();
        Buffer type = mCurrentToken.toString();

        // varName
        mCurrentToken = mTokenizer.getToken();
        mSymbolTable.define(mCurrentToken.toString(), type, SYMBOL_ARG);

        mCurrentToken = mTokenizer.getToken();
    }
}

void CompilationEngine::compileVarDec()
{
    // type varName (',' varName)* ';'
    mCurrentToken = mTokenizer.getToken();
    Buffer type = mCurrentToken.toString();

    mCurrentToken = mTokenizer.getToken();
    mSymbolTable.define(mCurrentToken.toString(), type, SYMBOL_VAR);

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isSymbol(','))
    {
        mCurrentToken = mTokenizer.getToken();
        mSymbolTable.define(mCurrentToken.toString(), type, SYMBOL_VAR);
        
        mCurrentToken = mTokenizer.getToken();
    }
}

void CompilationEngine::compileStatements()
{
    // statements: statement*
    // statement: letStatement | ifStatement | whileStatement | doStatement | returnStatement

    while (mCurrentToken.isKeyword())
    {
        switch (mCurrentToken.keyword)
        {
            case KEYWORD_LET:
                compileLet();
                mCurrentToken = mTokenizer.getToken();
                break;

            case KEYWORD_IF:
                compileIf();
                break;

            case KEYWORD_WHILE:
                compileWhile();
                mCurrentToken = mTokenizer.getToken();
                break;

            case KEYWORD_DO:
                compileDo();
                mCurrentToken = mTokenizer.getToken();
                break;

            case KEYWORD_RETURN:
                compileReturn();
                mCurrentToken = mTokenizer.getToken();
                break;

            default:
                unexpectedToken();
        }
    }
}

void CompilationEngine::compileDo()
{
    // 'do' subroutineCall ';'
    // subRoutineName
    readToken(TOKEN_IDENTIFIER);
    Buffer subRoutineName = mCurrentToken.toString();

    readToken(TOKEN_SYMBOL);
    compileSubroutineCall(subRoutineName, true);
    
    readSymbol(';');
}

void CompilationEngine::compileSubroutineCall(Buffer subRoutineName, bool isDo)
{
    Buffer className = {};
    int nArgs = 0;
    bool shouldRestoreThis = false;

    // Class or object call
    if (mCurrentToken.isSymbol('.'))
    {
        Buffer callerName = subRoutineName;
        readToken(TOKEN_IDENTIFIER);
        subRoutineName = mCurrentToken.toString();

        Symbol symbol = mSymbolTable.find(callerName);
        if (symbol.kind == SYMBOL_NONE)
        {
            className = callerName;
        }
        else
        {
            shouldRestoreThis = mIsMethod || mConstructor;
            if (shouldRestoreThis)
            {
                mVMWriter.writePush(SEGMENT_POINTER, 0);
            }

            className = symbol.type;
            ++nArgs;
            pushSymbol(symbol);
        }

        readToken(TOKEN_SYMBOL);
    }
    // local method call
    else
    {
        className = mClassName;
        ++nArgs;
        mVMWriter.writePush(SEGMENT_POINTER, 0);
    }

    verifySymbol('(');

    mCurrentToken = mTokenizer.getToken();

    nArgs += compileExpressionList();
    verifySymbol(')');

    mVMWriter.writeCall(className, subRoutineName, nArgs);

    // pop the stack with do calls to avoid bleeding garbage values
    if (isDo)
    {
        mVMWriter.writePop(SEGMENT_TEMP, 0);
        if (shouldRestoreThis)
        {
            mVMWriter.writePop(SEGMENT_POINTER, 0);
        }
    }
    else if (shouldRestoreThis)
    {
        mVMWriter.writePop(SEGMENT_TEMP, 0);
        mVMWriter.writePop(SEGMENT_POINTER, 0);
        mVMWriter.writePush(SEGMENT_TEMP, 0);
    }
}

void CompilationEngine::compileLet()
{
    // 'let' varName ('[' expression ']')? '=' expression ';'
    
    // varName
    readToken(TOKEN_IDENTIFIER);
    Buffer varName = mCurrentToken.toString();
    Symbol symbol = mSymbolTable.find(varName);

    readToken(TOKEN_SYMBOL);

    if (mCurrentToken.isSymbol('['))
    {
        mCurrentToken = mTokenizer.getToken();
        compileExpression();
        mVMWriter.writePop(SEGMENT_TEMP, 0);

        verifySymbol(']');
        mCurrentToken = mTokenizer.getToken();

        verifySymbol('=');

        mCurrentToken = mTokenizer.getToken();
        compileExpression();

        pushSymbol(symbol);
        mVMWriter.writePush(SEGMENT_TEMP, 0);
        mVMWriter.writeArithmetic(COMMAND_ADD);
        mVMWriter.writePop(SEGMENT_POINTER, 1);
        mVMWriter.writePop(SEGMENT_THAT, 0);
    }
    else
    {
        verifySymbol('=');

        mCurrentToken = mTokenizer.getToken();
        compileExpression();
        popToSymbol(symbol);
    }
    
    verifySymbol(';');
}

void CompilationEngine::compileWhile()
{
    // 'while' '(' expression ')' '{' statements '}'
    readSymbol('(');

    // Using the same name scheme as the example OS vm code
    char whileExpLabel[64];
    sprintf(whileExpLabel, "WHILE_EXP%d", mWhileCount);
    mVMWriter.writeLabel(whileExpLabel);

    mCurrentToken = mTokenizer.getToken();
    compileExpression();
    verifySymbol(')');
    readSymbol('{');

    char whileEndLabel[64];
    sprintf(whileEndLabel, "WHILE_END%d", mWhileCount);
    mVMWriter.writeArithmetic(COMMAND_NOT);
    mVMWriter.writeIf(whileEndLabel);

    mCurrentToken = mTokenizer.getToken();
    ++mWhileCount;
    compileStatements();
    verifySymbol('}');

    mVMWriter.writeGoto(whileExpLabel);
    mVMWriter.writeLabel(whileEndLabel);
}

void CompilationEngine::compileReturn()
{
    // 'return' expression? ';'
    mCurrentToken = mTokenizer.getToken();
    if (!mCurrentToken.isSymbol(';'))
    {
        compileExpression();
    }
    else
    {
        mVMWriter.writePush(SEGMENT_CONST, 0);
    }

    mVMWriter.writeReturn();
    verifySymbol(';');
}

void CompilationEngine::compileIf()
{
    // 'if' '(' expression ')' '{' statements '} ('else' '{' statements '})?
    readSymbol('(');

    mCurrentToken = mTokenizer.getToken();
    compileExpression();

    verifySymbol(')');
    readSymbol('{');

    char ifTrueLabel[64];
    sprintf(ifTrueLabel, "IF_TRUE%d", mIfCount);
    mVMWriter.writeIf(ifTrueLabel);

    char ifFalseLabel[64];
    sprintf(ifFalseLabel, "IF_FALSE%d", mIfCount);

    char ifEndLabel[64];
    sprintf(ifEndLabel, "IF_END%d", mIfCount);
    ++mIfCount;

    mVMWriter.writeGoto(ifFalseLabel);
    mVMWriter.writeLabel(ifTrueLabel);

    mCurrentToken = mTokenizer.getToken();
    compileStatements();

    verifySymbol('}');

    mCurrentToken = mTokenizer.getToken();
    if (mCurrentToken.isKeyword(KEYWORD_ELSE))
    {
        mVMWriter.writeGoto(ifEndLabel);
        mVMWriter.writeLabel(ifFalseLabel);

        readSymbol('{');

        mCurrentToken = mTokenizer.getToken();
        compileStatements();

        verifySymbol('}');

        mCurrentToken = mTokenizer.getToken();
        mVMWriter.writeLabel(ifEndLabel);
    }
    else
    {
        mVMWriter.writeLabel(ifFalseLabel);
    }

}

void CompilationEngine::compileExpression()
{
    // term (op term)*
    // op: '+ | '-' | '*' | '/' | '&' | '|' | '<' | '>' | '='
    compileTerm();

    while (isOperator())
    {
        char op = mCurrentToken.text[0];
        mCurrentToken = mTokenizer.getToken();
        compileTerm();

        switch (op)
        {
            case '+':
                mVMWriter.writeArithmetic(COMMAND_ADD);
                break;

            case '-':
                mVMWriter.writeArithmetic(COMMAND_SUB);
                break;

            case '*':
                mVMWriter.writeCall("Math.multiply", 2);
                break;

            case '/':
                mVMWriter.writeCall("Math.divide", 2);
                break;

            case '&':
                mVMWriter.writeArithmetic(COMMAND_AND);
                break;

            case '|':
                mVMWriter.writeArithmetic(COMMAND_OR);
                break;

            case '<':
                mVMWriter.writeArithmetic(COMMAND_LT);
                break;

            case '>':
                mVMWriter.writeArithmetic(COMMAND_GT);
                break;

            case '=':
                mVMWriter.writeArithmetic(COMMAND_EQ);
                break;
        }
    }
}

void CompilationEngine::compileTerm()
{
    // term: integerConstant | stringConstant | keywordConstant
    if (mCurrentToken.type == TOKEN_INTEGERCONST)
    {
        mVMWriter.writePush(SEGMENT_CONST, mCurrentToken.value);
        mCurrentToken = mTokenizer.getToken();
    }
    else if(mCurrentToken.type == TOKEN_STRINGCONST)
    {
        // Create string using sitring constructor and assign the values
        mVMWriter.writePush(SEGMENT_CONST, mCurrentToken.length);
        mVMWriter.writeCall("String.new", 1);
        for (int i = 0; i < mCurrentToken.length; ++i)
        {
            mVMWriter.writePush(SEGMENT_CONST, mCurrentToken.text[i]);
            mVMWriter.writeCall("String.appendChar", 2);
        }

        mCurrentToken = mTokenizer.getToken();
    }
    else if (mCurrentToken.isKeyword(KEYWORD_TRUE))
    {
        mVMWriter.writePush(SEGMENT_CONST, 1);
        mVMWriter.writeArithmetic(COMMAND_NEG);
        mCurrentToken = mTokenizer.getToken();
    }
    else if (mCurrentToken.isKeyword(KEYWORD_FALSE) || mCurrentToken.isKeyword(KEYWORD_NULL))
    {
        mVMWriter.writePush(SEGMENT_CONST, 0);
        mCurrentToken = mTokenizer.getToken();
    }
    else if (mCurrentToken.isKeyword(KEYWORD_THIS))
    {
        mVMWriter.writePush(SEGMENT_POINTER, 0);
        mCurrentToken = mTokenizer.getToken();
    }
    // term: '(' expression ')'
    else if (mCurrentToken.isSymbol('('))
    {
        mCurrentToken = mTokenizer.getToken();
        compileExpression();
        verifySymbol(')');
        mCurrentToken = mTokenizer.getToken();
    }
    // term: ('- | '~') term
    else if (mCurrentToken.isSymbol('-'))
    {
        mCurrentToken = mTokenizer.getToken();
        compileTerm();
        mVMWriter.writeArithmetic(COMMAND_NEG);
    }
    else if (mCurrentToken.isSymbol('~'))
    {
        mCurrentToken = mTokenizer.getToken();
        compileTerm();
        mVMWriter.writeArithmetic(COMMAND_NOT);
    }
    // term: varName | varName '[' expression ']' | subroutineCall
    else
    {
        Buffer varName = mCurrentToken.toString();

        mCurrentToken = mTokenizer.getToken();

        // term: varName '[' expression ']'
        if (mCurrentToken.isSymbol('['))
        {
            Symbol symbol = mSymbolTable.find(varName);

            mCurrentToken = mTokenizer.getToken();
            compileExpression();
            verifySymbol(']');

            pushSymbol(symbol);
            mVMWriter.writeArithmetic(COMMAND_ADD);
            mVMWriter.writePop(SEGMENT_POINTER, 1);
            mVMWriter.writePush(SEGMENT_THAT, 0);

            mCurrentToken = mTokenizer.getToken();
        }
        // term: subroutineCall
        else if (mCurrentToken.isSymbol('.') || mCurrentToken.isSymbol('('))
        {
            compileSubroutineCall(varName);
            mCurrentToken = mTokenizer.getToken();
        }
        // term: varName
        else
        {
            Symbol symbol = mSymbolTable.find(varName);
            pushSymbol(symbol);
        }
    }
}

int CompilationEngine::compileExpressionList()
{
    int numExpressions = 0;
    // (expression (', expression)*)?
    if (!mCurrentToken.isSymbol(')'))
    {
        compileExpression();
        ++numExpressions;

        while (mCurrentToken.isSymbol(','))
        {
            mCurrentToken = mTokenizer.getToken();
            compileExpression();
            ++numExpressions;
        }
    }

    return numExpressions;
}

void CompilationEngine::readKeyword(Keyword expectedKeyword)
{
    mCurrentToken = mTokenizer.getToken();
    if (!mCurrentToken.isKeyword(expectedKeyword))
    {
        unexpectedToken();
    }
}

void CompilationEngine::readSymbol(char expectedSymbol)
{
    mCurrentToken = mTokenizer.getToken();
    verifySymbol(expectedSymbol);
}

void CompilationEngine::readToken(enum TokenType expectedTokenType)
{
    mCurrentToken = mTokenizer.getToken();
    if (mCurrentToken.type != expectedTokenType)
    {
        unexpectedToken();
    }
}

void CompilationEngine::unexpectedToken()
{
    printf("Unexpected token in %s at line %d, col %d\n", mInputPath, mCurrentToken.lineNumber, mCurrentToken.column);
    exit(1);
}

void CompilationEngine::verifySymbol(char expectedSymbol)
{
    if (expectedSymbol && !mCurrentToken.isSymbol(expectedSymbol) || !mCurrentToken.isSymbol())
    {
        unexpectedToken();
    }
}

bool CompilationEngine::isOperator()
{
    if (!mCurrentToken.isSymbol())
    {
        return false;
    }

    switch (mCurrentToken.text[0])
    {
        case '+':
        case '-':
        case '*':
        case '/':
        case '&':
        case '|':
        case '<':
        case '>':
        case '=':
            return true;

        default:
            return false;
    }
}

bool CompilationEngine::isKeywordConstant()
{
    // keywordConstant: 'true' | 'false' | 'null' | 'this'
    if (!mCurrentToken.isKeyword())
    {
        return false;
    }

    switch (mCurrentToken.keyword)
    {
        case KEYWORD_FALSE:
        case KEYWORD_NULL:
        case KEYWORD_THIS:
            return true;

        default:
            return false;
    }
}

void CompilationEngine::pushSymbol(Symbol symbol)
{
    switch (symbol.kind)
    {
        case SYMBOL_STATIC:
            mVMWriter.writePush(SEGMENT_STATIC, symbol.index);
            break;
        case SYMBOL_VAR:
            mVMWriter.writePush(SEGMENT_LOCAL, symbol.index);
            break;
        case SYMBOL_FIELD:
            mVMWriter.writePush(SEGMENT_THIS, symbol.index);
            break;
        case SYMBOL_ARG:
            mVMWriter.writePush(SEGMENT_ARG, mIsMethod ? symbol.index + 1 : symbol.index);
            break;
    }
}

void CompilationEngine::popToSymbol(Symbol symbol)
{
    switch (symbol.kind)
    {
        case SYMBOL_STATIC:
            mVMWriter.writePop(SEGMENT_STATIC, symbol.index);
            break;
        case SYMBOL_VAR:
            mVMWriter.writePop(SEGMENT_LOCAL, symbol.index);
            break;
        case SYMBOL_FIELD:
            mVMWriter.writePop(SEGMENT_THIS, symbol.index);
            break;
        case SYMBOL_ARG:
            mVMWriter.writePop(SEGMENT_ARG, mIsMethod ? symbol.index + 1: symbol.index);
            break;
    }
}