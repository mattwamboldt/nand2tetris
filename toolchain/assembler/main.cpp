#include <cstdio>
#include <cstdlib>
#include <cstring>

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

bool isWhitespace(char v)
{
    return v <= ' ' && v > 0;
}

bool isEOL(char v)
{
    return v == '\r' || v == '\n';
}

int toDigit(char v)
{
    return v - '0';
}

enum Jump {
    JMP_NONE = 0,
    JMP_GT,
    JMP_EQ,
    JMP_GE,
    JMP_LT,
    JMP_NE,
    JMP_LE,
    JMP_ALL,
};

enum Type
{
    L_INSTRUCTION,
    A_INSTRUCTION,
    C_INSTRUCTION
};

struct Command {
    Type type;
    char* text;
    int length;
    int value;
};

struct Symbol {
    const char* text;
    int length;
    int value;
};

struct SymbolTable
{
    int count;
    Symbol symbols[32000];

    void Push(const char* text, int value)
    {
        symbols[count++] = { text, (int)strlen(text), value };
    }

    void Push(const char* text, int length, int value)
    {
        char* newText = (char*)malloc(length + 1);
        strncpy(newText, text, length);
        newText[length] = 0;
        symbols[count++] = { newText, length, value };
    }

    Symbol* Find(const char* text, int length)
    {
        for (int i = 0; i < count; ++i)
        {
            Symbol* symbol = symbols + i;
            if (symbol->length == length)
            {
                int c = 0;
                for (;c < length; ++c)
                {
                    if (symbol->text[c] != text[c])
                    {
                        break;
                    }
                }

                if (c == length)
                {
                    return symbol;
                }
            }
        }

        return 0;
    }
};

struct Parser {
    char* At;
};

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Usage: assembler <infile.asm> <outfile.hack>\n");
        return 0;
    }

    printf("Assemble %s into %s\n", argv[1], argv[2]);
    
    Buffer input = ReadWholeFile(argv[1]);
    if (!input.size) { return 0; }

    char* At = input.memory;
    // Eat any leading whitespace
    while (isWhitespace(*At))
    {
        ++At;
    }

    SymbolTable symbols = {};
    symbols.Push("SP", 0);
    symbols.Push("LCL", 1);
    symbols.Push("ARG", 2);
    symbols.Push("THIS", 3);
    symbols.Push("THAT", 4);
    symbols.Push("R0", 0);
    symbols.Push("R1", 1);
    symbols.Push("R2", 2);
    symbols.Push("R3", 3);
    symbols.Push("R4", 4);
    symbols.Push("R5", 5);
    symbols.Push("R6", 6);
    symbols.Push("R7", 7);
    symbols.Push("R8", 8);
    symbols.Push("R9", 9);
    symbols.Push("R10", 10);
    symbols.Push("R11", 11);
    symbols.Push("R12", 12);
    symbols.Push("R13", 13);
    symbols.Push("R14", 14);
    symbols.Push("R15", 15);
    symbols.Push("SCREEN", 0x4000);
    symbols.Push("KBD", 0x6000);

    int numCommands = 0;
    Command* commands = (Command*)malloc(sizeof(Command) * 100000);

    int lineNumber = 1;

    while (*At)
    {
        // Process comment, TODO: handle syntax error of a single /
        if (*(At + 1) == '/' && *At == '/')
        {
            while (!isEOL(*At))
            {
                ++At;
            }

            ++lineNumber;
        }
        else if (*At == '@')
        {
            Command command = {};
            command.type = A_INSTRUCTION;

            ++At;

            // Constant
            if (isDigit(*At))
            {
                do
                {
                    command.value *= 10;
                    command.value += toDigit(*At++);
                } while (isDigit(*At));
            }
            // symbol
            else
            {
                char* firstChar = At;
                while (!isWhitespace(*At))
                {
                    ++At;
                }

                int length = At - firstChar;
                Symbol* existingSymbol = symbols.Find(firstChar, length);
                if (existingSymbol)
                {
                    command.value = existingSymbol->value;
                }
                else
                {
                    command.text = firstChar;
                    command.length = length;
                }
            }

            while (!isEOL(*At))
            {
                ++At;
            }

            if (numCommands > 0)
            {
                Command lastCommand = commands[numCommands];
                if (lastCommand.type == command.type &&
                    lastCommand.length == command.length &&
                    lastCommand.text == command.text &&
                    lastCommand.value == command.value)
                {
                    printf("Duplicate Command\n");
                }
            }

            commands[numCommands++] = command;
        }
        else if (*At == '(')
        {
            char* firstChar = At + 1;
            while (!isWhitespace(*At) && *At != ')')
            {
                ++At;
            }

            int length = At - firstChar;
            Symbol* existingSymbol = symbols.Find(firstChar, length);
            if (existingSymbol)
            {
                printf("Duplicate Symbol, chars %d\n", firstChar - input.memory);
                return 0;
            }
            else
            {
                symbols.Push(firstChar, length, numCommands);
            }

            while (!isEOL(*At))
            {
                ++At;
            }
        }
        else
        {
            // C instruction
            // destination=computation;jump

            char* firstPart = At;
            while (*At
                && *At != '='
                && *At != ';'
                && !isEOL(*At))
            {
                ++At;
            }

            int dest = 0;
            Jump jump = JMP_NONE;

            if (*At == '=')
            {
                // dest portion
                while (firstPart < At)
                {
                    switch (*firstPart)
                    {
                    case 'D':
                        dest |= 0b010;
                        break;

                    case 'M':
                        dest |= 0b001;
                        break;

                    case 'A':
                        dest |= 0b100;
                        break;
                    }

                    ++firstPart;
                }

                ++At;
                firstPart = At;

                while (*At && *At != ';' && !isEOL(*At))
                {
                    ++At;
                }
            }

            int comp = -1;

            // process command
            if (*firstPart == '0')
            {
                comp = 0b101010;
            }
            else if (*firstPart == '1')
            {
                comp = 0b111111;
            }
            else if (*firstPart == '!')
            {
                ++firstPart;
                if (*firstPart == 'M')
                {
                    comp = 0b1110001;
                }
                else if (*firstPart == 'A')
                {
                    comp = 0b0110001;
                }
                else if (*firstPart == 'D')
                {
                    comp = 0b0001101;
                }
            }
            else if (*firstPart == '-')
            {
                ++firstPart;
                if (*firstPart == 'M')
                {
                    comp = 0b1110011;
                }
                else if (*firstPart == 'A')
                {
                    comp = 0b0110011;
                }
                else if (*firstPart == 'D')
                {
                    comp = 0b0001111;
                }
                else if (*firstPart == '1')
                {
                    comp = 0b0111010;
                }
            }
            else if (*firstPart == 'M')
            {
                ++firstPart;
                if (*firstPart == '+')
                {
                    ++firstPart;
                    if (*firstPart == 'D')
                    {
                        comp = 0b1000010;
                    }
                    else if (*firstPart == '1')
                    {
                        comp = 0b1110111;
                    }
                }
                else if (*firstPart == '-')
                {
                    ++firstPart;
                    if (*firstPart == 'D')
                    {
                        comp = 0b1000111;
                    }
                    else if (*firstPart == '1')
                    {
                        comp = 0b1110010;
                    }
                }
                else if (*firstPart == '&')
                {
                    comp = 0b1000000;
                }
                else if (*firstPart == '|')
                {
                    comp = 0b1010101;
                }
                else
                {
                    comp = 0b1110000;
                }
            }
            else if (*firstPart == 'D')
            {
                ++firstPart;
                if (*firstPart == '+')
                {
                    ++firstPart;
                    if (*firstPart == 'M')
                    {
                        comp = 0b1000010;
                    }
                    else if (*firstPart == 'A')
                    {
                        comp = 0b0000010;
                    }
                    else if (*firstPart == '1')
                    {
                        comp = 0b0011111;
                    }
                }
                else if (*firstPart == '-')
                {
                    ++firstPart;
                    if (*firstPart == 'M')
                    {
                        comp = 0b1010011;
                    }
                    else if (*firstPart == 'A')
                    {
                        comp = 0b0010011;
                    }
                    else if (*firstPart == '1')
                    {
                        comp = 0b0001110;
                    }
                }
                else if (*firstPart == '&')
                {
                    ++firstPart;
                    if (*firstPart == 'M')
                    {
                        comp = 0b1000000;
                    }
                    else if (*firstPart == 'A')
                    {
                        comp = 0b0000000;
                    }
                }
                else if (*firstPart == '|')
                {
                    ++firstPart;
                    if (*firstPart == 'M')
                    {
                        comp = 0b1010101;
                    }
                    else if (*firstPart == 'A')
                    {
                        comp = 0b0010101;
                    }
                }
                else
                {
                    comp = 0b0001100;
                }
            }
            else if (*firstPart == 'A')
            {
                ++firstPart;
                if (*firstPart == '+')
                {
                    ++firstPart;
                    if (*firstPart == 'D')
                    {
                        comp = 0b0000010;
                    }
                    else if (*firstPart == '1')
                    {
                        comp = 0b0110111;
                    }
                }
                else if (*firstPart == '-')
                {
                    ++firstPart;
                    if (*firstPart == 'D')
                    {
                        comp = 0b0000111;
                    }
                    else if (*firstPart == '1')
                    {
                        comp = 0b0110010;
                    }
                }
                else if (*firstPart == '&')
                {
                    comp = 0b0000000;
                }
                else if (*firstPart == '|')
                {
                    comp = 0b0010101;
                }
                else
                {
                    comp = 0b0110000;
                }
            }

            // Add jmp
            if (*At == ';')
            {
                At++;
                if (strncmp("JGT", At, 3) == 0)
                {
                    jump = JMP_GT;
                }
                else if (strncmp("JEQ", At, 3) == 0)
                {
                    jump = JMP_EQ;
                }
                else if (strncmp("JGE", At, 3) == 0)
                {
                    jump = JMP_GE;
                }
                else if (strncmp("JLT", At, 3) == 0)
                {
                    jump = JMP_LT;
                }
                else if (strncmp("JNE", At, 3) == 0)
                {
                    jump = JMP_NE;
                }
                else if (strncmp("JLE", At, 3) == 0)
                {
                    jump = JMP_LE;
                }
                else if (strncmp("JMP", At, 3) == 0)
                {
                    jump = JMP_ALL;
                }

                At += 3;
            }

            Command command = {};
            command.type = C_INSTRUCTION;
            command.value = jump | (dest << 3) | (comp << 6) | (0b111 << 13);
            if (numCommands > 0)
            {
                Command lastCommand = commands[numCommands];
                if (lastCommand.type == command.type &&
                    lastCommand.length == command.length &&
                    lastCommand.text == command.text &&
                    lastCommand.value == command.value)
                {
                    printf("Duplicate Command\n");
                }
            }

            while (!isEOL(*At))
            {
                ++At;
            }

            commands[numCommands++] = command;
        }

        // Eat any remaining whitespace to next command
        while (isWhitespace(*At))
        {
            At++;
        }
    }

    Buffer output = {};
    long outSize = numCommands * 17;
    output.memory = (char*)malloc(outSize);

    int variable = 16;
    for (int i = 0; i < numCommands; ++i)
    {
        Command command = commands[i];
        int value = command.value;
        if (command.type == A_INSTRUCTION && command.text)
        {
            Symbol* existingSymbol = symbols.Find(command.text, command.length);
            if (existingSymbol)
            {
                value = existingSymbol->value;
            }
            else
            {
                value = variable++;
                symbols.Push(command.text, command.length, value);
            }
        }

        for (int shift = 15; shift >= 0; --shift)
        {
            if (((value >> shift) & 1) > 0)
            {
                output.memory[output.size++] = '1';
            }
            else
            {
                output.memory[output.size++] = '0';
            }
        }

        output.memory[output.size++] = '\n';
    }

    WriteWholeFile(argv[2], output.memory, output.size);
    return 0;
}