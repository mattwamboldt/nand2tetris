#include <cstring>
#include "jacktokenizer.h"
#include "util.h"

bool Token::equals(const char* match)
{
    for (int i = 0; i < length; ++i, ++match)
    {
        if (*match == 0 || *match != text[i])
        {
            return false;
        }
    }

    return *match == 0;
}

void Token::print(FILE* outputFile)
{
    switch (type)
    {
    case TOKEN_KEYWORD:
        fprintf(outputFile, "<keyword> %.*s </keyword>\n", length, text);
        break;

    case TOKEN_SYMBOL:
    {
        char symbol = text[0];
        if (text[0] == '<')
        {
            fprintf(outputFile, "<symbol> &lt; </symbol>\n");
        }
        else if (text[0] == '>')
        {
            fprintf(outputFile, "<symbol> &gt; </symbol>\n");
        }
        else if (text[0] == '&')
        {
            fprintf(outputFile, "<symbol> &amp; </symbol>\n");
        }
        else
        {
            fprintf(outputFile, "<symbol> %c </symbol>\n", text[0]);
        }
    }
    break;

    case TOKEN_INTEGERCONST:
        fprintf(outputFile, "<integerConstant> %d </integerConstant>\n", value);
        break;

    case TOKEN_STRINGCONST:
        fprintf(outputFile, "<stringConstant> %.*s </stringConstant>\n", length, text);
        break;

    case TOKEN_IDENTIFIER:
        fprintf(outputFile, "<identifier> %.*s </identifier>\n", length, text);
        break;
    }
}

bool isDigit(char v)
{
    return v >= '0' && v <= '9';
}

bool isUpper(char v)
{
    return v >= 'A' && v <= 'Z';
}

bool isLower(char v)
{
    return v >= 'a' && v <= 'z';
}

bool isEOL(char v)
{
    return v == '\r' || v == '\n';
}

bool isWhitespace(char v)
{
    return v == ' ' || v == '\t';
}

int toDigit(char v)
{
    return v - '0';
}

JackTokenizer::JackTokenizer(char* path)
    : mAt(0), mPath(path), mLine(1), mCol(1), mCurrentSymbol(0)
{
    Buffer input = readWholeFile(mPath);
    if (!input.size) { return; }
    mAt = input.memory;
    mCurrentSymbol = *mAt;
    eatAllWhitespace();
}

void JackTokenizer::advance(int amount)
{
    mAt += amount;
    mCol += amount;
    mCurrentSymbol = *mAt;
}

void JackTokenizer::eatAllWhitespace()
{
    while (mCurrentSymbol)
    {
        if (isWhitespace(mCurrentSymbol))
        {
            advance();
        }
        else if (mAt[0] == '\r' && mAt[1] == '\n')
        {
            advance(2);
            mCol = 1;
            ++mLine;
        }
        else if (isEOL(mCurrentSymbol))
        {
            advance();
            mCol = 1;
            ++mLine;
        }
        // single line comments
        else if (mAt[0] == '/' && mAt[1] == '/')
        {
            advance(2);
            while (mCurrentSymbol && !isEOL(mCurrentSymbol))
            {
                advance();
            }
        }
        // block comment
        else if (mAt[0] == '/' && mAt[1] == '*')
        {
            advance(2);
            while (mCurrentSymbol && !(mAt[0] == '*' && mAt[1] == '/'))
            {
                advance();
            }

            if (mCurrentSymbol)
            {
                advance(2);
            }
        }
        else
        {
            break;
        }
    }
}

Token JackTokenizer::getToken()
{
    eatAllWhitespace();

    Token token = {};
    token.text = mAt;
    token.length = 1;
    token.lineNumber = mLine;
    token.column = mCol;

    switch (mCurrentSymbol)
    {
        case 0:
            token.type = TOKEN_EOF;
            break;

        case '{':
        case '}':
        case '(':
        case ')':
        case '[':
        case ']':
        case '.':
        case ',':
        case ';':
        case '+':
        case '-':
        case '*':
        case '/':
        case '&':
        case '|':
        case '<':
        case '>':
        case '=':
        case '~':
            advance();
            token.type = TOKEN_SYMBOL;
            break;

        default:
        {
            if (isDigit(mCurrentSymbol))
            {
                token.type = TOKEN_INTEGERCONST;
                do
                {
                    token.value *= 10;
                    token.value += toDigit(mCurrentSymbol);
                    advance();
                } while (isDigit(mCurrentSymbol));

                // TODO: Handle out of range and unexpected symbols
                token.length = mAt - token.text;
            }
            else if (mCurrentSymbol == '"')
            {
                token.type = TOKEN_STRINGCONST;
                advance();
                token.text = mAt;
                // TODO: Handle unexpected end line
                while (mCurrentSymbol && mCurrentSymbol != '"')
                {
                    advance();
                }

                token.length = mAt - token.text;
                advance();
            }
            else
            {
                token.type = TOKEN_IDENTIFIER;

                do
                {
                    advance();
                }
                while (mCurrentSymbol && (isUpper(mCurrentSymbol) || isLower(mCurrentSymbol) || isDigit(mCurrentSymbol) || mCurrentSymbol == '_'));

                token.length = mAt - token.text;

                if (token.equals("class"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_CLASS;
                }
                else if (token.equals("constructor"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_CONSTRUCTOR;
                }
                else if (token.equals("function"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_FUNCTION;
                }
                else if (token.equals("method"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_METHOD;
                }
                else if (token.equals("field"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_FIELD;
                }
                else if (token.equals("static"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_STATIC;
                }
                else if (token.equals("var"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_VAR;
                }
                else if (token.equals("int"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_INT;
                }
                else if (token.equals("char"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_CHAR;
                }
                else if (token.equals("boolean"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_BOOLEAN;
                }
                else if (token.equals("void"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_VOID;
                }
                else if (token.equals("true"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_TRUE;
                }
                else if (token.equals("false"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_FALSE;
                }
                else if (token.equals("null"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_NULL;
                }
                else if (token.equals("this"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_THIS;
                }
                else if (token.equals("let"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_LET;
                }
                else if (token.equals("do"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_DO;
                }
                else if (token.equals("if"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_IF;
                }
                else if (token.equals("else"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_ELSE;
                }
                else if (token.equals("while"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_WHILE;
                }
                else if (token.equals("return"))
                {
                    token.type = TOKEN_KEYWORD;
                    token.keyword = KEYWORD_RETURN;
                }
            }
        }
        break;
    }

    return token;
}

void JackTokenizer::writeToFile()
{
    char outputPath[256] = {};
    strcpy(outputPath, mPath);

    char* ext = extension(outputPath);
    strcpy(ext, "T.xml");

    FILE* outputFile = fopen(outputPath, "w");
    if (!outputFile)
    {
        printf("Failed to open output file\n");
        return;
    }

    fprintf(outputFile, "<tokens>\n");

    Token nextToken = getToken();
    while (nextToken.type != TOKEN_EOF)
    {
        nextToken.print(outputFile);

        eatAllWhitespace();
        nextToken = getToken();
    }

    fprintf(outputFile, "</tokens>\n");
}