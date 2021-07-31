#pragma once
#include <cstdio>
#include "util.h"

enum VMSegment
{
    SEGMENT_CONST,
    SEGMENT_ARG,
    SEGMENT_LOCAL,
    SEGMENT_STATIC,
    SEGMENT_THIS,
    SEGMENT_THAT,
    SEGMENT_POINTER,
    SEGMENT_TEMP
};

enum VMCommand
{
    COMMAND_ADD,
    COMMAND_SUB,
    COMMAND_NEG,
    COMMAND_EQ,
    COMMAND_GT,
    COMMAND_LT,
    COMMAND_AND,
    COMMAND_OR,
    COMMAND_NOT
};

class VMWriter
{
    public:
        VMWriter(char* outputPath);

        void writePush(VMSegment segment, int index);
        void writePop(VMSegment segment, int index);
        void writeArithmetic(VMCommand command);
        void writeLabel(char* label);
        void writeLabel(Buffer label);
        void writeGoto(char* label);
        void writeGoto(Buffer label);
        void writeIf(char* label);
        void writeIf(Buffer label);
        void writeCall(const char* label, int nArgs);
        void writeCall(Buffer className, Buffer functionName, int nArgs);
        void writeFunction(Buffer className, Buffer functionName, int nLocals);
        void writeReturn();
        void close();

    private:
        FILE* mOutputFile;
};