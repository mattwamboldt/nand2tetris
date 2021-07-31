#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include "util.h"

Buffer readWholeFile(char* path)
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

bool Buffer::equals(Buffer match)
{
    if (size != match.size)
    {
        return false;
    }

    for (long i = 0; i < size; ++i)
    {
        if (memory[i] != match.memory[i])
        {
            return false;
        }
    }

    return true;
}