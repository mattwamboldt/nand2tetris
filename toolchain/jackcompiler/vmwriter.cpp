#include "vmwriter.h"

VMWriter::VMWriter(char* outputPath)
{
    mOutputFile = fopen(outputPath, "w");
    if (!mOutputFile)
    {
        printf("Failed to open output file: %s\n", outputPath);
    }
}

void VMWriter::writePush(VMSegment segment, int index)
{
    switch (segment)
    {
        case SEGMENT_CONST:
            fprintf(mOutputFile, "push constant %d\n", index);
            break;

        case SEGMENT_ARG:
            fprintf(mOutputFile, "push argument %d\n", index);
            break;

        case SEGMENT_LOCAL:
            fprintf(mOutputFile, "push local %d\n", index);
            break;

        case SEGMENT_STATIC:
            fprintf(mOutputFile, "push static %d\n", index);
            break;

        case SEGMENT_THIS:
            fprintf(mOutputFile, "push this %d\n", index);
            break;

        case SEGMENT_THAT:
            fprintf(mOutputFile, "push that %d\n", index);
            break;

        case SEGMENT_POINTER:
            fprintf(mOutputFile, "push pointer %d\n", index);
            break;

        case SEGMENT_TEMP:
            fprintf(mOutputFile, "push temp %d\n", index);
            break;
    }
}

void VMWriter::writePop(VMSegment segment, int index)
{
    switch (segment)
    {
        case SEGMENT_CONST:
            fprintf(mOutputFile, "pop constant %d\n", index);
            break;

        case SEGMENT_ARG:
            fprintf(mOutputFile, "pop argument %d\n", index);
            break;

        case SEGMENT_LOCAL:
            fprintf(mOutputFile, "pop local %d\n", index);
            break;

        case SEGMENT_STATIC:
            fprintf(mOutputFile, "pop static %d\n", index);
            break;

        case SEGMENT_THIS:
            fprintf(mOutputFile, "pop this %d\n", index);
            break;

        case SEGMENT_THAT:
            fprintf(mOutputFile, "pop that %d\n", index);
            break;

        case SEGMENT_POINTER:
            fprintf(mOutputFile, "pop pointer %d\n", index);
            break;

        case SEGMENT_TEMP:
            fprintf(mOutputFile, "pop temp %d\n", index);
            break;
    }
}

void VMWriter::writeArithmetic(VMCommand command)
{
    switch (command)
    {
        case COMMAND_ADD:
            fprintf(mOutputFile, "add\n");
            break;

        case COMMAND_SUB:
            fprintf(mOutputFile, "sub\n");
            break;

        case COMMAND_NEG:
            fprintf(mOutputFile, "neg\n");
            break;

        case COMMAND_EQ:
            fprintf(mOutputFile, "eq\n");
            break;

        case COMMAND_GT:
            fprintf(mOutputFile, "gt\n");
            break;

        case COMMAND_LT:
            fprintf(mOutputFile, "lt\n");
            break;

        case COMMAND_AND:
            fprintf(mOutputFile, "and\n");
            break;

        case COMMAND_OR:
            fprintf(mOutputFile, "or\n");
            break;

        case COMMAND_NOT:
            fprintf(mOutputFile, "not\n");
            break;
    }
}

void VMWriter::writeLabel(char* label)
{
    fprintf(mOutputFile, "label %s\n", label);
}

void VMWriter::writeLabel(Buffer label)
{
    fprintf(mOutputFile, "label %.*s\n", label.size, label.memory);
}

void VMWriter::writeGoto(char* label)
{
    fprintf(mOutputFile, "goto %s\n", label);
}

void VMWriter::writeGoto(Buffer label)
{
    fprintf(mOutputFile, "goto %.*s\n", label.size, label.memory);
}

void VMWriter::writeIf(char* label)
{
    fprintf(mOutputFile, "if-goto %s\n", label);
}

void VMWriter::writeIf(Buffer label)
{
    fprintf(mOutputFile, "if-goto %.*s\n", label.size, label.memory);
}

void VMWriter::writeCall(const char* label, int nArgs)
{
    fprintf(mOutputFile, "call %s %d\n", label, nArgs);
}

void VMWriter::writeCall(Buffer className, Buffer functionName, int nArgs)
{
    fprintf(mOutputFile, "call %.*s.%.*s %d\n", className.size, className.memory, functionName.size, functionName.memory, nArgs);
}

void VMWriter::writeFunction(Buffer className, Buffer functionName, int nLocals)
{
    fprintf(mOutputFile, "function %.*s.%.*s %d\n", className.size, className.memory, functionName.size, functionName.memory, nLocals);
}

void VMWriter::writeReturn()
{
    fprintf(mOutputFile, "return\n");
}

void VMWriter::close()
{
    fclose(mOutputFile);
}
