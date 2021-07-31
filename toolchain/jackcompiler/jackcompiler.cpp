#include <windows.h>
#include "util.h"
#include "compilationengine.h"

void compileFile(char* path)
{
    char outputPath[256] = {};
    strcpy(outputPath, path);

    char* ext = extension(outputPath);
    strcpy(ext, ".vm");

    CompilationEngine parser = CompilationEngine(path, outputPath);
    parser.compileClass();
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
                    compileFile(filePath);
                }
            } while (FindNextFileA(hFind, &fdFile));

            FindClose(hFind);
        }
    }
    else
    {
        compileFile(path);
    }

    return 0;
}