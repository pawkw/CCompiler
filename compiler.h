#ifndef PAWC_COMPILER
#define PAWC_COMPILER

#include <stdio.h>
#include <stdbool.h>

struct pos
{
    int line;
    int col;
    const char* filename;
};

enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

struct token
{
    int type;
    int flags;

    union
    {
        char cval;
        const char* sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void* any;
    };

    // True if there is whitespace between this and the following token.
    bool whitespace;

    // A pointer to the first token in the series of tokens within the brackets.
    // For debugging.
    const char* between_brackets; 

};

enum {
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

struct compile_process
{
    // Compile flags.
    int flags;

    struct compile_process_input_file
    {
        FILE *fp;
        const char* abs_path;
    } cfile;

    FILE* ofile;
};

// compiler.c
int compile_file(const char* filename, const char *out_filename, int flags);

// cprocess.c
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags);

#endif