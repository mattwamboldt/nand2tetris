#include <stdlib.h>
#include "compilationengine.h"

void printIndent(FILE* outputFile, int indent)
{
    fprintf(outputFile, "%*s", indent, "");
}

CompilationEngine::CompilationEngine(char* inputPath, char* outputPath)
    :mTokenizer(inputPath), mCurrentToken(), mInputPath(inputPath), mIndent(0)
{
    mOutputFile = fopen(outputPath, "w");
    if (!mOutputFile)
    {
        printf("Failed to open output file: %s\n", outputPath);
    }
}

// type: 'int' | 'char' | 'boolean' | className
// className: identifier
// subroutineName: identifier
// varName: identifier
// subroutineCall: subroutineName '(' expressionList ') | (className | varName) '.' subroutineName '(' expressionList ')

void CompilationEngine::compileClass()
{
    // 'class' className '{' classVarDec* subRoutineDec* '}'

    fprintf(mOutputFile, "<class>\n");
    mIndent = 2;

    readKeyword(Keyword::KEYWORD_CLASS);
    printCurrentToken();

    readToken(TokenType::TOKEN_IDENTIFIER);
    printCurrentToken();

    readSymbol('{');
    printCurrentToken();

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
    printCurrentToken();

    mIndent = 0;
    fprintf(mOutputFile, "</class>\n");
}

void CompilationEngine::compileClassVarDec()
{
    // ('static' | 'field') type varName (',' varName)* ';'
    printOpenNode("classVarDec");

    printCurrentToken();

    // TODO: Verify Type
    mCurrentToken = mTokenizer.getToken();
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isSymbol(','))
    {
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
    }

    printCurrentToken();
    printCloseNode("classVarDec");
}

void CompilationEngine::compileSubroutine()
{
    // subroutineDec: ('constructor' | 'function' | 'method') ('void' | type) subRoutineName '(' parameterList ')' subroutineBody
    printOpenNode("subroutineDec");

    printCurrentToken();

    // return type
    mCurrentToken = mTokenizer.getToken();
    printCurrentToken();

    // subroutineName
    mCurrentToken = mTokenizer.getToken();
    printCurrentToken();

    readSymbol('(');
    printCurrentToken();
    compileParameterList();
    printCurrentToken();

    // subroutineBody: '{' varDec* statements '}'
    printOpenNode("subroutineBody");

    readSymbol('{');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isKeyword(KEYWORD_VAR))
    {
        compileVarDec();
        mCurrentToken = mTokenizer.getToken();
    }

    compileStatements();
    verifySymbol('}');
    printCurrentToken();

    printCloseNode("subroutineBody");
    printCloseNode("subroutineDec");
}

void CompilationEngine::compileParameterList()
{
    // ((type varName) (',' type varName)*)?
    printOpenNode("parameterList");

    mCurrentToken = mTokenizer.getToken();
    // TODO: loop
    if (mCurrentToken.type == TOKEN_IDENTIFIER || mCurrentToken.isKeyword())
    {
        // TODO: handle type
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
    }

    while (mCurrentToken.isSymbol(','))
    {
        printCurrentToken();

        // type
        mCurrentToken = mTokenizer.getToken();
        printCurrentToken();

        // varName
        mCurrentToken = mTokenizer.getToken();
        printCurrentToken();

        mCurrentToken = mTokenizer.getToken();
    }

    printCloseNode("parameterList");
}

void CompilationEngine::compileVarDec()
{
    // type varName (',' varName)* ';'
    printOpenNode("varDec");

    printCurrentToken();

    // TODO: Verify Type
    mCurrentToken = mTokenizer.getToken();
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    while (mCurrentToken.isSymbol(','))
    {
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
    }

    printCurrentToken();

    printCloseNode("varDec");
}

void CompilationEngine::compileStatements()
{
    // statements: statement*
    // statement: letStatement | ifStatement | whileStatement | doStatement | returnStatement
    printOpenNode("statements");

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

    printCloseNode("statements");
}

void CompilationEngine::compileDo()
{
    // 'do' subroutineCall ';'
    printOpenNode("doStatement");

    printCurrentToken();

    // subRoutineName
    readToken(TOKEN_IDENTIFIER);
    printCurrentToken();

    readToken(TOKEN_SYMBOL);

    if (mCurrentToken.isSymbol('.'))
    {
        printCurrentToken();
        readToken(TOKEN_IDENTIFIER);
        printCurrentToken();
        readToken(TOKEN_SYMBOL);
    }

    verifySymbol('(');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    compileExpressionList();
    verifySymbol(')');
    printCurrentToken();

    readSymbol(';');
    printCurrentToken();

    printCloseNode("doStatement");
}

void CompilationEngine::compileLet()
{
    // 'let' varName ('[' expression ']')? '=' expression ';'
    printOpenNode("letStatement");

    printCurrentToken();

    // varName
    readToken(TOKEN_IDENTIFIER);
    printCurrentToken();

    readToken(TOKEN_SYMBOL);

    if (mCurrentToken.isSymbol('['))
    {
        printCurrentToken();

        mCurrentToken = mTokenizer.getToken();
        compileExpression();

        verifySymbol(']');
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
    }

    verifySymbol('=');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    compileExpression();

    verifySymbol(';');
    printCurrentToken();

    printCloseNode("letStatement");
}

void CompilationEngine::compileWhile()
{
    // 'while' '(' expression ')' '{' statements '}'
    printOpenNode("whileStatement");

    printCurrentToken();
    readSymbol('(');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    compileExpression();
    verifySymbol(')');
    printCurrentToken();

    readSymbol('{');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    compileStatements();
    verifySymbol('}');
    printCurrentToken();

    printCloseNode("whileStatement");
}

void CompilationEngine::compileReturn()
{
    // 'return' expression? ';'
    printOpenNode("returnStatement");

    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    if (!mCurrentToken.isSymbol(';'))
    {
        compileExpression();
    }

    verifySymbol(';');
    printCurrentToken();

    printCloseNode("returnStatement");
}

void CompilationEngine::compileIf()
{
    // 'if' '(' expression ')' '{' statements '} ('else' '{' statements '})?
    printOpenNode("ifStatement");

    printCurrentToken();

    readSymbol('(');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    compileExpression();

    verifySymbol(')');
    printCurrentToken();

    readSymbol('{');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    compileStatements();

    verifySymbol('}');
    printCurrentToken();

    mCurrentToken = mTokenizer.getToken();
    if (mCurrentToken.isKeyword(KEYWORD_ELSE))
    {
        printCurrentToken();
        readSymbol('{');
        printCurrentToken();

        mCurrentToken = mTokenizer.getToken();
        compileStatements();

        verifySymbol('}');
        printCurrentToken();

        mCurrentToken = mTokenizer.getToken();
    }

    printCloseNode("ifStatement");
}

void CompilationEngine::compileExpression()
{
    // term (op term)*
    // op: '+ | '-' | '*' | '/' | '&' | '|' | '<' | '>' | '='
    printOpenNode("expression");

    compileTerm();

    while (isOperator())
    {
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
        compileTerm();
    }

    printCloseNode("expression");
}

void CompilationEngine::compileTerm()
{
    // term: integerConstant | stringConstant | keywordConstant | varName | varName '[' expression ']' | subroutineCall | '(' expression ')' | unaryOp term
    
    printOpenNode("term");
    printCurrentToken();
    
    if (mCurrentToken.type == TOKEN_INTEGERCONST
        || mCurrentToken.type == TOKEN_STRINGCONST
        || isKeywordConstant())
    {
        mCurrentToken = mTokenizer.getToken();
    }
    else if (mCurrentToken.isSymbol('('))
    {
        mCurrentToken = mTokenizer.getToken();
        compileExpression();
        verifySymbol(')');
        printCurrentToken();
        mCurrentToken = mTokenizer.getToken();
    }
    // unaryOp: '- | '~'
    else if (mCurrentToken.isSymbol('-') || mCurrentToken.isSymbol('~'))
    {
        mCurrentToken = mTokenizer.getToken();
        compileTerm();
    }
    else
    {
        mCurrentToken = mTokenizer.getToken();

        // varName '[' expression ']'
        if (mCurrentToken.isSymbol('['))
        {
            printCurrentToken();
            mCurrentToken = mTokenizer.getToken();
            compileExpression();
            verifySymbol(']');
            printCurrentToken();

            mCurrentToken = mTokenizer.getToken();
        }
        else 
        {
            if (mCurrentToken.isSymbol('.'))
            {
                printCurrentToken();
                readToken(TOKEN_IDENTIFIER);
                printCurrentToken();
                readSymbol('(');
            }

            if (mCurrentToken.isSymbol('('))
            {
                printCurrentToken();
                mCurrentToken = mTokenizer.getToken();
                compileExpressionList();
                verifySymbol(')');
                printCurrentToken();
                mCurrentToken = mTokenizer.getToken();
            }
        }
    }

    printCloseNode("term");
}

void CompilationEngine::compileExpressionList()
{
    // (expression (', expression)*)?
    printOpenNode("expressionList");

    if (!mCurrentToken.isSymbol(')'))
    {
        compileExpression();
        while (mCurrentToken.isSymbol(','))
        {
            printCurrentToken();
            mCurrentToken = mTokenizer.getToken();
            compileExpression();
        }
    }

    printCloseNode("expressionList");
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

void CompilationEngine::printOpenNode(const char* name)
{
    printIndent(mOutputFile, mIndent);
    fprintf(mOutputFile, "<%s>\n", name);
    mIndent += 2;
}

void CompilationEngine::printCloseNode(const char* name)
{
    mIndent -= 2;
    printIndent(mOutputFile, mIndent);
    fprintf(mOutputFile, "</%s>\n", name);
}

void CompilationEngine::printCurrentToken()
{
    printIndent(mOutputFile, mIndent);
    mCurrentToken.print(mOutputFile);
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