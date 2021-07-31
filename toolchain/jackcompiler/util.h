#pragma once

struct Buffer
{
    long size;
    char* memory;

    Buffer();
    bool equals(Buffer match);
};

Buffer readWholeFile(char* path);
bool isDirectory(char* path);
char* extension(char* path);
char* basename(char* path);