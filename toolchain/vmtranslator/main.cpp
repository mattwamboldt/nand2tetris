#include <cstring>
#include <cstdio>
#include <cstdlib>

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
    TokenType type;
    char* text;
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
    FILE* outputFile;

    bool Open(char* sourcePath)
    {
        char outputPath[256] = {};
        strcpy(outputPath, sourcePath);
        int length = strlen(outputPath);
        char* end = outputPath + length - 1;

        bool hitDirectory = false;
        while (*end)
        {
            if (*end == '\\' || *end == '/')
            {
                hitDirectory = true;
                break;
            }
            else if (*end == '.')
            {
                // TODO: compare extension to make sure we're cool
                break;
            }

            --end;
        }

        if (hitDirectory)
        {
            // TODO: process directories
            end = outputPath + length;
            // for each *.vm file in directory, run translate
            return false;
        }

        char* dir = end;
        while (dir > outputPath)
        {
            if (*dir == '\\' || *dir == '/')
            {
                ++dir;
                break;
            }

            --dir;
        }

        strncpy(currentModule, dir, end - dir);
        currentModule[end - dir] = 0;

        strcpy(end, ".asm");

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

    void Pop(Token segment, Token index)
    {
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
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M-1\n");
        fprintf(outputFile, "D=M\n");

        // *(mem[R15]) = D
        fprintf(outputFile, "@R15\n");
        fprintf(outputFile, "A=M\n");
        fprintf(outputFile, "M=D\n");
    }

    void ArithmeticTwoParam(char op)
    {
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M-1\n");
        fprintf(outputFile, "D=M\n"); // D is y
        fprintf(outputFile, "A=A-1\n"); // M is x and return location
        fprintf(outputFile, "M=M%cD\n", op);
    }

    void ArithmeticOneParam(char op)
    {
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "A=M-1\n");
        fprintf(outputFile, "M=%cM\n", op);
    }

    void Compare(CompareOps type)
    {
        fprintf(outputFile, "@SP\n");
        fprintf(outputFile, "AM=M-1\n");
        fprintf(outputFile, "D=M\n");
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
};

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: VMTranslator <file.vm | folder>\n");
        return 0;
    }

    char* path = argv[1];

    CodeWriter writer;
    if (!writer.Open(path))
    {
        printf("Failed to open output file");
        return 1;
    }

    // TODO: handle folder
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
                    writer.Push(segment, index);
                }
                else if (token.Equals("pop"))
                {
                    Token segment = tokenizer.GetToken();
                    Token index = tokenizer.GetToken();
                    writer.Pop(segment, index);
                }
                else if (token.Equals("add"))
                {
                    writer.ArithmeticTwoParam('+');
                }
                else if (token.Equals("sub"))
                {
                    writer.ArithmeticTwoParam('-');
                }
                else if (token.Equals("and"))
                {
                    writer.ArithmeticTwoParam('&');
                }
                else if (token.Equals("or"))
                {
                    writer.ArithmeticTwoParam('|');
                }
                else if (token.Equals("neg"))
                {
                    writer.ArithmeticOneParam('-');
                }
                else if (token.Equals("not"))
                {
                    writer.ArithmeticOneParam('!');
                }
                else if (token.Equals("eq"))
                {
                    writer.Compare(EQUAL);
                }
                else if (token.Equals("gt"))
                {
                    writer.Compare(GREATER_THAN);
                }
                else if (token.Equals("lt"))
                {
                    writer.Compare(LESS_THAN);
                }
                else if (token.Equals("label"))
                {
                    printf("%d: %.*s\n", token.type, token.length, token.text);
                }
                else if (token.Equals("goto"))
                {
                    printf("%d: %.*s\n", token.type, token.length, token.text);
                }
                else if (token.Equals("if-goto"))
                {
                    printf("%d: %.*s\n", token.type, token.length, token.text);
                }
                else if (token.Equals("function"))
                {
                    printf("%d: %.*s\n", token.type, token.length, token.text);
                }
                else if (token.Equals("call"))
                {
                    printf("%d: %.*s\n", token.type, token.length, token.text);
                }
                else if (token.Equals("return"))
                {
                    printf("%d: %.*s\n", token.type, token.length, token.text);
                }
            }
            break;

            default:
                printf("%d: %.*s\n", token.type, token.length, token.text);
                break;
        }
    }

    return 0;
}