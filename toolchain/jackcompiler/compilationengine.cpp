#include <stdlib.h>
#include "compilationengine.h"

void printIndent(FILE* outputFile, int indent)
{
    fprintf(outputFile, "%*s", indent, "");
}

const char* kindStr[] = { "none", "static", "field", "arg", "var" };

CompilationEngine::CompilationEngine(char* inputPath, char* outputPath)
    :mTokenizer(inputPath), mVMWriter(outputPath), mCurrentToken(), mInputPath(inputPath), mClassName()
{
    
}

// type: 'int' | 'char' | 'boolean' | className
// className: identifier
// subroutineName: identifier
// varName: identifier
// subroutineCall: subroutineName '(' expressionList ') | (className | varName) '.' subroutineName '(' expressionList ')

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
    }

    compileStatements();

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
    compileSubroutineCall(subRoutineName);

    // pop the stack with do calls to avoid bleeding garbage values
    mVMWriter.writePop(SEGMENT_TEMP, 0);
    
    readSymbol(';');
}

void CompilationEngine::compileSubroutineCall(Buffer subRoutineName)
{
    Buffer className = {};
    bool isMethod = false;

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
            className = symbol.type;
            isMethod = true;
        }

        readToken(TOKEN_SYMBOL);
    }
    // local method call
    else
    {
        className = mClassName;
        isMethod = true;
    }

    verifySymbol('(');

    mCurrentToken = mTokenizer.getToken();
    int nArgs = compileExpressionList();
    verifySymbol(')');

    if (isMethod) ++nArgs;
    mVMWriter.writeCall(className, subRoutineName, nArgs);
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

        verifySymbol(']');
        mCurrentToken = mTokenizer.getToken();
    }

    verifySymbol('=');
    
    mCurrentToken = mTokenizer.getToken();
    compileExpression();

    verifySymbol(';');
}

void CompilationEngine::compileWhile()
{
    // 'while' '(' expression ')' '{' statements '}'
    readSymbol('(');

    mCurrentToken = mTokenizer.getToken();
    compileExpression();
    verifySymbol(')');

    readSymbol('{');

    mCurrentToken = mTokenizer.getToken();
    compileStatements();
    verifySymbol('}');
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

    mCurrentToken = mTokenizer.getToken();
    compileStatements();

    verifySymbol('}');

    mCurrentToken = mTokenizer.getToken();
    if (mCurrentToken.isKeyword(KEYWORD_ELSE))
    {
        readSymbol('{');

        mCurrentToken = mTokenizer.getToken();
        compileStatements();

        verifySymbol('}');

        mCurrentToken = mTokenizer.getToken();
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
    else if(mCurrentToken.type == TOKEN_STRINGCONST || isKeywordConstant())
    {
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
    else if (mCurrentToken.isSymbol('-') || mCurrentToken.isSymbol('~'))
    {
        mCurrentToken = mTokenizer.getToken();
        compileTerm();
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
        case KEYWORD_TRUE:
        case KEYWORD_FALSE:
        case KEYWORD_NULL:
        case KEYWORD_THIS:
            return true;

        default:
            return false;
    }
}