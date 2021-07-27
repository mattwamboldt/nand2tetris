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

void WriteWholeFile(char* path, void* memory, long numBytes)
{
    FILE* file = fopen(path, "w");
    if (file)
    {
        fwrite(memory, 1, numBytes, file);
        fclose(file);
    }
    else
    {
        printf("Failed to open file: %s\n", path);
    }
}

bool isDigit(char v)
{
    return v >= '0' && v <= '9';
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

enum TokenType
{
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,

    TOKEN_EOF,
};

struct Token
{
    enum TokenType type;
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

    void EatAllWhitespace()
    {
        while (*At)
        {
            if (isWhitespace(*At))
            {
                ++At;
            }
            else if (At[0] == '/' && At[1] == '/')
            {
                while (*At && !isEOL(*At)) ++At;
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
            case 0: token.type = TOKEN_EOF; break;
            default:
            {
                if (isDigit(*At))
                {
                    token.type = TOKEN_NUMBER;
                    do
                    {
                        token.value *= 10;
                        token.value += toDigit(*At++);
                    } while (isDigit(*At));
                    token.length = At - token.text;
                }
                else
                {
                    token.type = TOKEN_IDENTIFIER;
                    while (!isWhitespace(*At)) ++At;
                    token.length = At - token.text;
                }
            } break;
        }

        return token;
    }
};

enum CompareOps {
    LESS_THAN,
    GREATER_THAN,
    EQUAL,
    NUM_COMPARE_OPS
};

struct CodeWriter
{
    int compareCounts[NUM_COMPARE_OPS] = { 0, 0, 0 };
    const char* compareStrings[NUM_COMPARE_OPS] = {"LT", "GT", "EQ"};
    
    char currentModule[256];
    Token scope = {};
    FILE* outputFile;
    bool comments = true;
    int callCount = 0;

    void SetCurrentModule(char* path)
    {
        char* filename = basename(path);
        char* ext = extension(filename);

        strncpy(currentModule, filename, ext - filename);
        currentModule[ext - filename] = 0;
    }

    bool Open(char* sourcePath, bool isDirectory)
    {
        char outputPath[256] = {};
        if (isDirectory)
        {
            sprintf(outputPath, "%s\\%s", sourcePath, basename(sourcePath));
        }
        else
        {
            strcpy(outputPath, sourcePath);
        }

        char* ext = extension(outputPath);
        strcpy(ext, ".asm");

        outputFile = fopen(outputPath, "w");
        if (!outputFile)
        {
            return false;
        }

        return true;
    }

    void BasicSegmentAddress(const char* segment, int index)
    {
        fprintf(outputFile, "@%s\n", segment);
        fprintf(outputFile, "AD=M\n");
        if (index)
        {
            fprintf(outputFile, "@%d\n", index);
            fprintf(outputFile, "AD=A+D\n");
        }
    }

    void BasicSegmentValue(const char* segment, int index)
    {
        BasicSegmentAddress(segment, index);
        fprintf(outputFile, "D=M\n");
    }

    void Push(Token segment, Token index)
    {
        if (comments)
        {
            fprintf(outputFile, "// push %.*s %d\n", segment.length, segment.text, index.value);
        }

        // Load segment value into D resgister
        if (segment.Equals("constant"))
        {
            // D = value
            fprintf(outputFile, "@%d\n", index.value);
            fprintf(outputFile, "D=A\n");
        }
        else if (segment.Equals("local"))
        {
            BasicSegmentValue("LCL", index.value);
        }
        else if (segment.Equals("argument"))
        {
            BasicSegmentValue("ARG", index.value);
        }
        else if (segment.Equals("this"))
        {
            BasicSegmentValue("THIS", index.value);
        }
        else if (segment.Equals("that"))
        {
            BasicSegmentValue("THAT", index.value);
        }
        else if (segment.Equals("temp"))
        {
            fprintf(outputFile, "@R%d\n", index.value + 5);
            fprintf(outputFile, "D=M\n");
        }
        else if (segment.Equals("static"))
        {
            fprintf(outputFile, "@%s.%d\n", currentModule, index.value);
            fprintf(outputFile, "D=M\n");
        }
        else if (segment.Equals("pointer"))
        {
            if (index.value == 0)
            {
                fprintf(outputFile, "@THIS\n");
            }
            else
            {
                fprintf(outputFile, "@THAT\n");
            }

            fprintf(outputFile, "D=M\n");
        }
        else
        {
            printf("Unrecognized segment: %.*s\n", segment.length, segment.text);
            exit(0);
            return;
        }

        // set top stack to d
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M+1\n");
        fprintf(outputFile, "A=A-1\n");
        fprintf(outputFile, "M=D\n");
    }

    void GlobalPop()
    {
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M-1\n");
        fprintf(outputFile, "D=M\n");
    }

    void Pop(Token segment, Token index)
    {
        if (comments)
        {
            fprintf(outputFile, "// pop %.*s %d\n", segment.length, segment.text, index.value);
        }

        if (segment.Equals("local"))
        {
            BasicSegmentAddress("LCL", index.value);
        }
        else if (segment.Equals("argument"))
        {
            BasicSegmentAddress("ARG", index.value);
        }
        else if (segment.Equals("this"))
        {
            BasicSegmentAddress("THIS", index.value);
        }
        else if (segment.Equals("that"))
        {
            BasicSegmentAddress("THAT", index.value);
        }
        else if (segment.Equals("temp"))
        {
            fprintf(outputFile, "@R%d\n", index.value + 5);
            fprintf(outputFile, "D=A\n");
        }
        else if (segment.Equals("static"))
        {
            fprintf(outputFile, "@%s.%d\n", currentModule, index.value);
            fprintf(outputFile, "D=A\n");
        }
        else if (segment.Equals("pointer"))
        {
            if (index.value == 0)
            {
                fprintf(outputFile, "@THIS\n");
            }
            else
            {
                fprintf(outputFile, "@THAT\n");
            }

            fprintf(outputFile, "D=A\n");
        }
        else
        {
            printf("Unrecognized segment: %.*s\n", segment.length, segment.text);
            exit(0);
            return;
        }

        // mem[R15] = D
        fprintf(outputFile, "@R15\n");
        fprintf(outputFile, "M=D\n");

        // D = mem[--mem[sp]]
        GlobalPop();

        // *(mem[R15]) = D
        fprintf(outputFile, "@R15\n");
        fprintf(outputFile, "A=M\n");
        fprintf(outputFile, "M=D\n");
    }

    void ArithmeticTwoParam(char op)
    {
        if (comments)
        {
            fprintf(outputFile, "// x %c y\n", op);
        }

        GlobalPop();
        fprintf(outputFile, "A=A-1\n"); // M is x and return location
        fprintf(outputFile, "M=M%cD\n", op);
    }

    void ArithmeticOneParam(char op)
    {
        if (comments)
        {
            fprintf(outputFile, "// %cy\n", op);
        }

        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "A=M-1\n");
        fprintf(outputFile, "M=%cM\n", op);
    }

    void Compare(CompareOps type)
    {
        if (comments)
        {
            switch (type)
            {
                case LESS_THAN:
                    fprintf(outputFile, "// x < y\n");
                    break;

                case GREATER_THAN:
                    fprintf(outputFile, "// x > y\n");
                    break;

                case EQUAL:
                    fprintf(outputFile, "// x == y\n");
                    break;
            }
        }

        GlobalPop();
        fprintf(outputFile, "A=A-1\n");
        fprintf(outputFile, "D=M-D\n");
        fprintf(outputFile, "M=-1\n");
        fprintf(outputFile, "@END_%s%d\n", compareStrings[type], compareCounts[type]);
        fprintf(outputFile, "D;J%s\n", compareStrings[type]);
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "A=M-1\n");
        fprintf(outputFile, "M=0\n");
        fprintf(outputFile, "(END_%s%d)\n", compareStrings[type], compareCounts[type]);
        ++compareCounts[type];
    }

    void Label(Token name)
    {
        if (scope.text)
        {
            fprintf(outputFile, "(%.*s$%.*s)\n", scope.length, scope.text, name.length, name.text);
        }
        else
        {
            fprintf(outputFile, "(%.*s)\n", name.length, name.text);
        }
    }

    void Goto(Token location)
    {
        if (comments)
        {
            fprintf(outputFile, "// goto %.*s\n", location.length, location.text);
        }

        if (scope.text)
        {
            fprintf(outputFile, "@%.*s$%.*s\n", scope.length, scope.text, location.length, location.text);
        }
        else
        {
            fprintf(outputFile, "@%.*s\n", location.length, location.text);
        }

        fprintf(outputFile, "0;JMP\n");
    }

    void IfGoto(Token location)
    {
        if (comments)
        {
            fprintf(outputFile, "// if-goto %.*s\n", location.length, location.text);
        }

        GlobalPop();
        if (scope.text)
        {
            fprintf(outputFile, "@%.*s$%.*s\n", scope.length, scope.text, location.length, location.text);
        }
        else
        {
            fprintf(outputFile, "@%.*s\n", location.length, location.text);
        }

        fprintf(outputFile, "D;JNE\n");
    }

    void Function(Token name, Token nLocals)
    {
        if (comments)
        {
            fprintf(outputFile, "// function %.*s %d\n", name.length, name.text, nLocals.value);
        }

        scope = {};
        Label(name);
        scope = name;

        for (int i = 0; i < nLocals.value; ++i)
        {
            fprintf(outputFile, "@SP\n");
            fprintf(outputFile, "AM=M+1\n");
            fprintf(outputFile, "A=A-1\n");
            fprintf(outputFile, "M=0\n");
        }
    }

    void Call(Token name, Token nArgs)
    {
        if (comments)
        {
            fprintf(outputFile, "// call %.*s %d\n", name.length, name.text, nArgs.value);
        }

        fprintf(outputFile, "@RETURN_ADDRESS_%d\n", callCount);
        fprintf(outputFile, "D=A\n");

        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "A=M\n");
        fprintf(outputFile, "M=D\n");

        fprintf(outputFile, "@LCL\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M+1\n");
        fprintf(outputFile, "M=D\n");
        
        fprintf(outputFile, "@ARG\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M+1\n");
        fprintf(outputFile, "M=D\n");

        fprintf(outputFile, "@THIS\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M+1\n");
        fprintf(outputFile, "M=D\n");

        fprintf(outputFile, "@THAT\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M+1\n");
        fprintf(outputFile, "M=D\n");

        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "MD=M+1\n");

        fprintf(outputFile, "@LCL\n");
        fprintf(outputFile, "M=D\n");

        fprintf(outputFile, "@5\n");
        fprintf(outputFile, "D=D-A\n");

        if (nArgs.value)
        {
            fprintf(outputFile, "@%d\n", nArgs.value);
            fprintf(outputFile, "D=D-A\n");
        }

        fprintf(outputFile, "@ARG\n");
        fprintf(outputFile, "M=D\n");
        
        fprintf(outputFile, "@%.*s\n", name.length, name.text);
        fprintf(outputFile, "0;JMP\n");

        fprintf(outputFile, "(RETURN_ADDRESS_%d)\n", callCount);
        ++callCount;
    }

    void Return()
    {
        if (comments)
        {
            fprintf(outputFile, "// return\n");
        }

        // result = pop()
        GlobalPop();
        fprintf(outputFile, "@R13\n");
        fprintf(outputFile, "M=D\n");

        // endSP = arg + 1
        fprintf(outputFile, "@ARG\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@R14\n");
        fprintf(outputFile, "M=D+1\n");

        // sp = lcl
        fprintf(outputFile, "@LCL\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "M=D\n");

        // that = pop()
        GlobalPop();
        fprintf(outputFile, "@THAT\n");
        fprintf(outputFile, "M=D\n");

        // this = pop()
        GlobalPop();
        fprintf(outputFile, "@THIS\n");
        fprintf(outputFile, "M=D\n");

        // arg = pop()
        GlobalPop();
        fprintf(outputFile, "@ARG\n");
        fprintf(outputFile, "M=D\n");

        // lcl = pop()
        GlobalPop();
        fprintf(outputFile, "@LCL\n");
        fprintf(outputFile, "M=D\n");

        // returnAddress = pop()
        GlobalPop();
        fprintf(outputFile, "@R15\n");
        fprintf(outputFile, "M=D\n");

        // sp = endSP
        fprintf(outputFile, "@R14\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "M=D\n");

        // push result
        fprintf(outputFile, "@R13\n");
        fprintf(outputFile, "D=M\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "A=M-1\n");
        fprintf(outputFile, "M=D\n");

        // goto ret
        fprintf(outputFile, "@R15\n");
        fprintf(outputFile, "A=M\n");
        fprintf(outputFile, "0;JMP\n");
    }

    void Bootstrap()
    {
        fprintf(outputFile, "@256\n");
        fprintf(outputFile, "D=A\n");
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "M=D\n");

        Token name;
        name.text = "Sys.init";
        name.length = 8;

        Token argCount = {};
        Call(name, argCount);
    }
};

void TranslateFile(char* path, CodeWriter* writer)
{
    Buffer input = ReadWholeFile(path);

    Tokenizer tokenizer;
    tokenizer.At = input.memory;

    bool parsing = true;
    while (parsing)
    {
        Token token = tokenizer.GetToken();
        switch (token.type)
        {
            case TOKEN_EOF:
                parsing = false;
                break;

            case TOKEN_IDENTIFIER:
            {
                if (token.Equals("push"))
                {
                    Token segment = tokenizer.GetToken();
                    Token index = tokenizer.GetToken();
                    writer->Push(segment, index);
                }
                else if (token.Equals("pop"))
                {
                    Token segment = tokenizer.GetToken();
                    Token index = tokenizer.GetToken();
                    writer->Pop(segment, index);
                }
                else if (token.Equals("add"))
                {
                    writer->ArithmeticTwoParam('+');
                }
                else if (token.Equals("sub"))
                {
                    writer->ArithmeticTwoParam('-');
                }
                else if (token.Equals("and"))
                {
                    writer->ArithmeticTwoParam('&');
                }
                else if (token.Equals("or"))
                {
                    writer->ArithmeticTwoParam('|');
                }
                else if (token.Equals("neg"))
                {
                    writer->ArithmeticOneParam('-');
                }
                else if (token.Equals("not"))
                {
                    writer->ArithmeticOneParam('!');
                }
                else if (token.Equals("eq"))
                {
                    writer->Compare(EQUAL);
                }
                else if (token.Equals("gt"))
                {
                    writer->Compare(GREATER_THAN);
                }
                else if (token.Equals("lt"))
                {
                    writer->Compare(LESS_THAN);
                }
                else if (token.Equals("label"))
                {
                    writer->Label(tokenizer.GetToken());
                }
                else if (token.Equals("goto"))
                {
                    writer->Goto(tokenizer.GetToken());
                }
                else if (token.Equals("if-goto"))
                {
                    writer->IfGoto(tokenizer.GetToken());
                }
                else if (token.Equals("function"))
                {
                    Token name = tokenizer.GetToken();
                    Token nLocals = tokenizer.GetToken();
                    writer->Function(name, nLocals);
                }
                else if (token.Equals("call"))
                {
                    Token name = tokenizer.GetToken();
                    Token nArgs = tokenizer.GetToken();
                    writer->Call(name, nArgs);
                }
                else if (token.Equals("return"))
                {
                    writer->Return();
                }
            }
            break;

            default:
                printf("%d: %.*s\n", token.type, token.length, token.text);
                break;
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: VMTranslator <file.vm | folder>\n");
        return 0;
    }

    char* path = argv[1];
    bool isDir = isDirectory(path);
    CodeWriter writer;
    if (!writer.Open(path, isDir))
    {
        printf("Failed to open output file");
        return 1;
    }

    if (isDir)
    {
        writer.Bootstrap();

        char searchPath[MAX_PATH];
        char filePath[MAX_PATH];
        sprintf(searchPath, "%s\\*.vm", path);

        WIN32_FIND_DATAA fdFile;
        HANDLE hFind = FindFirstFileA(searchPath, &fdFile);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    sprintf(filePath, "%s\\%s", path, fdFile.cFileName);
                    writer.SetCurrentModule(fdFile.cFileName);
                    TranslateFile(filePath, &writer);
                }
            }
            while (FindNextFileA(hFind, &fdFile));

            FindClose(hFind);
        }
    }
    else
    {
        writer.SetCurrentModule(path);
        TranslateFile(path, &writer);
    }

    return 0;
}