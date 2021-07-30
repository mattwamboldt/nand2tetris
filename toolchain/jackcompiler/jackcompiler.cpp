#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <windows.h>

bool isDirectory(char* path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

char* extension(char* path)
{
    char* ext = 0;
    while (*path)
    {
        if (*path == '.')
        {
            ext = path;
        }

        ++path;
    }

    if (!ext)
    {
        return path;
    }

    return ext;
}

char* basename(char* path)
{
    char* filename = path;
    while (*path)
    {
        if (*path == '\\' || *path == '/')
        {
            filename = path + 1;
        }

        ++path;
    }

    return filename;
}

struct Buffer
{
    long size;
    char* memory;
};

Buffer ReadWholeFile(char* path)
{
    Buffer result = {};
    FILE* file = fopen(path, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        result.size = ftell(file);
        fseek(file, 0, SEEK_SET);
        int pos = ftell(file);

        result.memory = (char*)malloc(result.size + 1);
        size_t readResult = fread(result.memory, 1, result.size, file);
        if (readResult != result.size)
        {
            perror("The following error occurred");
            exit(3);
        }

        result.memory[result.size] = 0;
        fclose(file);
    }
    else
    {
        printf("Failed to open file: %s\n", path);
    }

    return result;
}

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
    const char* text;
    int length;
    int value;

    bool Equals(const char* match)
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
};

struct Tokenizer 
{
    char* At;

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
        return v == ' ' || v == '\t' || isEOL(v);
    }

    int toDigit(char v)
    {
        return v - '0';
    }

    void EatAllWhitespace()
    {
        while (*At)
        {
            if (isWhitespace(*At))
            {
                ++At;
            }
            // single line comments
            else if (At[0] == '/' && At[1] == '/')
            {
                At += 2;
                while (*At && !isEOL(*At)) ++At;
            }
            // block comment
            else if (At[0] == '/' && At[1] == '*')
            {
                At += 2;
                while (*At && !(At[0] == '*' && At[1] == '/')) ++At;
                if (*At) At += 2;
            }
            else
            {
                break;
            }
        }
    }

    Token GetToken()
    {
        EatAllWhitespace();

        Token token = {};
        token.text = At;
        token.length = 1;

        switch (*At)
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
                ++At;
                token.type = TOKEN_SYMBOL;
                break;

            default:
            {
                if (isDigit(*At))
                {
                    token.type = TOKEN_INTEGERCONST;
                    do
                    {
                        token.value *= 10;
                        token.value += toDigit(*At++);
                    }
                    while (isDigit(*At));
                    
                    // TODO: Handle out of range and unexpected symbols
                    token.length = At - token.text;
                }
                else if (*At == '"')
                {
                    token.type = TOKEN_STRINGCONST;
                    token.text = ++At;
                    // TODO: Handle unexpected end line
                    while (*At && *At != '"') ++At;
                    token.length = At - token.text;
                    ++At;
                }
                else
                {
                    ++At;
                    token.type = TOKEN_IDENTIFIER;
                    while (*At && (isUpper(*At) || isLower(*At) || isDigit(*At) || *At == '_')) ++At;
                    token.length = At - token.text;

                    if (token.Equals("class"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_CLASS;
                    }
                    else if (token.Equals("constructor"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_CONSTRUCTOR;
                    }
                    else if (token.Equals("function"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_FUNCTION;
                    }
                    else if (token.Equals("method"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_METHOD;
                    }
                    else if (token.Equals("field"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_FIELD;
                    }
                    else if (token.Equals("static"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_STATIC;
                    }
                    else if (token.Equals("var"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_VAR;
                    }
                    else if (token.Equals("int"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_INT;
                    }
                    else if (token.Equals("char"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_CHAR;
                    }
                    else if (token.Equals("boolean"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_BOOLEAN;
                    }
                    else if (token.Equals("void"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_VOID;
                    }
                    else if (token.Equals("true"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_TRUE;
                    }
                    else if (token.Equals("false"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_FALSE;
                    }
                    else if (token.Equals("null"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_NULL;
                    }
                    else if (token.Equals("this"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_THIS;
                    }
                    else if (token.Equals("let"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_LET;
                    }
                    else if (token.Equals("do"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_DO;
                    }
                    else if (token.Equals("if"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_IF;
                    }
                    else if (token.Equals("else"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_ELSE;
                    }
                    else if (token.Equals("while"))
                    {
                        token.type = TOKEN_KEYWORD;
                        token.keyword = KEYWORD_WHILE;
                    }
                    else if (token.Equals("return"))
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
};

void printIndent(FILE* outputFile, int indent)
{
    fprintf(outputFile, "%*s", indent, "");
}

void printToken(FILE* outputFile, Token token)
{
    switch (token.type)
    {
        case TOKEN_KEYWORD:
            fprintf(outputFile, "<keyword> %.*s </keyword>\n", token.length, token.text);
            break;
        
        case TOKEN_SYMBOL:
            {
                char symbol = token.text[0];
                if (token.text[0] == '<')
                {
                    fprintf(outputFile, "<symbol> &lt; </symbol>\n");
                }
                else if (token.text[0] == '>')
                {
                    fprintf(outputFile, "<symbol> &gt; </symbol>\n");
                }
                else if (token.text[0] == '&')
                {
                    fprintf(outputFile, "<symbol> &amp; </symbol>\n");
                }
                else
                {
                    fprintf(outputFile, "<symbol> %c </symbol>\n", token.text[0]);
                }
            }
            break;

        case TOKEN_INTEGERCONST:
            fprintf(outputFile, "<integerConstant> %d </integerConstant>\n", token.value);
            break;

        case TOKEN_STRINGCONST:
            fprintf(outputFile, "<stringConstant> %.*s </stringConstant>\n", token.length, token.text);
            break;

        case TOKEN_IDENTIFIER:
            fprintf(outputFile, "<identifier> %.*s </identifier>\n", token.length, token.text);
            break;
    }
}

void CompileFile(char* path)
{
    Buffer input = ReadWholeFile(path);
    if (!input.size) { return; }

    char outputPath[256] = {};
    strcpy(outputPath, path);

    char* ext = extension(outputPath);
    strcpy(ext, "T.xml");

    FILE* outputFile = fopen(outputPath, "w");
    if (!outputFile)
    {
        printf("Failed to open output file\n");
        return;
    }

    fprintf(outputFile, "<tokens>\n");

    Tokenizer tokenizer = {};
    tokenizer.At = input.memory;
    tokenizer.EatAllWhitespace();
    Token nextToken = tokenizer.GetToken();
    while (nextToken.type != TOKEN_EOF)
    {
        printToken(outputFile, nextToken);

        tokenizer.EatAllWhitespace();
        nextToken = tokenizer.GetToken();
    }

    fprintf(outputFile, "</tokens>\n");
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: jackcompiler <file.jack | folder>\n");
        return 0;
    }

    char* path = argv[1];
    bool isDir = isDirectory(path);
    if (isDir)
    {
        char searchPath[MAX_PATH];
        char filePath[MAX_PATH];
        sprintf(searchPath, "%s\\*.jack", path);

        WIN32_FIND_DATAA fdFile;
        HANDLE hFind = FindFirstFileA(searchPath, &fdFile);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    sprintf(filePath, "%s\\%s", path, fdFile.cFileName);
                    CompileFile(filePath);
                }
            } while (FindNextFileA(hFind, &fdFile));

            FindClose(hFind);
        }
    }
    else
    {
        CompileFile(path);
    }

    return 0;
}