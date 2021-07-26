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
    FILE* file = fopen(path, "r");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        result.size = ftell(file);
        rewind(file);

        result.memory = (char*)malloc(result.size + 1);
        memset(result.memory, 0, result.size + 1);
        fread(result.memory, 1, result.size, file);
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

    Buffer output = {};
    output.memory = (char*)malloc(input.size + 1);

    // Eat any leading whitespace
    while (isWhitespace(*input.memory))
    {
        input.memory++;
    }

    int currentInstruction = 1;

    while (*input.memory)
    {
        // Process comment, TODO: handle syntax error of a single /
        if (*(input.memory + 1) == '/' &&
            *input.memory == '/')
        {
            while (!isEOL(*input.memory))
            {
                input.memory++;
            }
        }
        else if (*input.memory == '@')
        {
            ++input.memory;
            int value = 0;

            // Constant
            if (isDigit(*input.memory))
            {
                do
                {
                    value *= 10;
                    value += toDigit(*input.memory++);
                } while (isDigit(*input.memory));
            }
            // symbol lookup
            else
            {

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
            ++currentInstruction;
        }
        else if (*input.memory == '(')
        {
            // TODO: set up a label
            while (!isWhitespace(*input.memory++))
            {
                output.memory[output.size++] = *input.memory++;
            }

            output.memory[output.size++] = *input.memory++;
        }
        else
        {
            // C instruction
            // destination=computation;jump

            char* firstPart = input.memory;
            while (*input.memory != '='
                && *input.memory != ';'
                && !isEOL(*input.memory))
            {
                ++input.memory;
            }

            int dest = 0;
            Jump jump = JMP_NONE;

            if (*input.memory == '=')
            {
                // dest portion
                while (firstPart < input.memory)
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

                ++input.memory;
                firstPart = input.memory;

                while (*input.memory != ';' && !isEOL(*input.memory))
                {
                    ++input.memory;
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

            if (comp < 0)
            {
                printf("Failed to parse: Line %d\n", currentInstruction);
                return 0;
            }

            // Add jmp
            if (*input.memory == ';')
            {
                input.memory++;
                if (strncmp("JGT", input.memory, 3) == 0)
                {
                    jump = JMP_GT;
                }
                else if (strncmp("JEQ", input.memory, 3) == 0)
                {
                    jump = JMP_EQ;
                }
                else if (strncmp("JGE", input.memory, 3) == 0)
                {
                    jump = JMP_GE;
                }
                else if (strncmp("JLT", input.memory, 3) == 0)
                {
                    jump = JMP_LT;
                }
                else if (strncmp("JNE", input.memory, 3) == 0)
                {
                    jump = JMP_NE;
                }
                else if (strncmp("JLE", input.memory, 3) == 0)
                {
                    jump = JMP_LE;
                }
                else if (strncmp("JMP", input.memory, 3) == 0)
                {
                    jump = JMP_ALL;
                }

                input.memory += 3;
            }

            int instruction = 0;
            instruction = jump | (dest << 3) | (comp << 6) | (0b111 << 13);

            for (int shift = 15; shift >= 0; --shift)
            {
                if (((instruction >> shift) & 1) > 0)
                {
                    output.memory[output.size++] = '1';
                }
                else
                {
                    output.memory[output.size++] = '0';
                }
            }

            output.memory[output.size++] = '\n';
            ++currentInstruction;
        }

        // Eat any remaining whitespace to next command
        while (isWhitespace(*input.memory))
        {
            input.memory++;
        }
    }

    WriteWholeFile(argv[2], output.memory, output.size);
    return 0;
}